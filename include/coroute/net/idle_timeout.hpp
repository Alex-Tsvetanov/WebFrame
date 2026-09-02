#pragma once

#include "coroute/net/io_context.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <memory>
#include <mutex>
#include <utility>

namespace coroute::net
{

	// Makes Connection::set_timeout mean something.
	//
	// Every backend stores the value passed to set_timeout and none of them enforces it,
	// while App::handle_connection sets a keep-alive timeout and then handles
	// Error::is_timeout() in its error path. So the caller was written against an
	// interface that does not work, and the branch that reads as "the client went quiet,
	// close the connection" is unreachable.
	//
	// A decorator rather than four backend implementations. Enforcing this inside each
	// event loop means per-descriptor deadline structures in four places, three of which
	// cannot be compiled on the machine this was written on, and the result would be a
	// timeout that behaves differently per backend, which is the one thing the
	// readiness-versus-completion comparison cannot afford. Above the backend it is one
	// implementation, identical everywhere by construction. TlsConnection and
	// PrefaceConnection already establish the wrapping-Connection pattern here.
	//
	// Idleness rather than a deadline per operation: the timer re-arms itself and
	// compares against the last time any byte moved, so the per-read cost is one relaxed
	// atomic store rather than a scheduling call. A connection that is being used never
	// touches the timer queue at all.
	class IdleTimeout final : public Connection
	{
	public:
		// A period of zero wraps without arming anything, which is how it is switched
		// off. Enabling is a construction-time decision; set_timeout afterwards changes
		// the period but cannot start a timer that was never armed.
		IdleTimeout(std::unique_ptr<Connection> inner, IoContext& ctx, std::chrono::milliseconds period)
			: inner_(std::move(inner)), state_(std::make_shared<State>())
		{
			state_->inner = inner_.get();
			state_->period_ms.store(period.count(), std::memory_order_relaxed);
			touch();

			if (period > std::chrono::milliseconds::zero())
			{
				arm(ctx, state_, period);
			}
		}

		~IdleTimeout() override
		{
			// After this the timer finds a null inner and stops re-arming. Under the
			// mutex because the timer may be inside close() on another thread right now,
			// and the connection it is closing is about to be destroyed.
			std::lock_guard<std::mutex> lock(state_->mutex);
			state_->inner = nullptr;
		}

		Task<ReadResult> async_read(void* buffer, size_t len) override
		{
			touch();
			auto result = co_await inner_->async_read(buffer, len);
			co_return substitute(std::move(result));
		}

		Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override
		{
			touch();
			auto result = co_await inner_->async_read_until(buffer, len, delimiter);
			co_return substitute(std::move(result));
		}

		Task<WriteResult> async_write(const void* buffer, size_t len) override
		{
			touch();
			co_return co_await inner_->async_write(buffer, len);
		}

		Task<WriteResult> async_write_all(const void* buffer, size_t len) override
		{
			touch();
			co_return co_await inner_->async_write_all(buffer, len);
		}

		Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) override
		{
			touch();
			co_return co_await inner_->async_transmit_file(file, offset, length);
		}

		void close() override { inner_->close(); }

		bool is_open() const noexcept override { return inner_->is_open(); }

		// Now does what its name says. Still passed down, so a backend that grows a real
		// implementation later agrees with this one rather than competing with it.
		void set_timeout(std::chrono::milliseconds timeout) override
		{
			state_->period_ms.store(timeout.count(), std::memory_order_relaxed);
			inner_->set_timeout(timeout);
		}

		std::string remote_address() const override { return inner_->remote_address(); }

		uint16_t remote_port() const noexcept override { return inner_->remote_port(); }

		void set_cancellation_token(CancellationToken token) override { inner_->set_cancellation_token(token); }

		// Whether this connection was closed for going quiet, as opposed to by the peer.
		[[nodiscard]] bool expired() const noexcept { return state_->expired.load(std::memory_order_relaxed); }

	private:
		struct State
		{
			std::mutex mutex;      // guards inner, and nothing else
			Connection* inner = nullptr;
			std::atomic<std::int64_t> last_activity_ns{0};
			std::atomic<std::int64_t> period_ms{0};
			std::atomic<bool> expired{false};
		};

		// The coarse monotonic clock where there is one, because this decides a timeout
		// measured in seconds and steady_clock is not free.
		//
		// steady_clock::now() is clock_gettime(CLOCK_MONOTONIC), which the vDSO usually
		// serves without entering the kernel. Usually is not always: where the TSC has
		// been disqualified the vDSO cannot serve it and every call becomes a syscall.
		// Measured on the Linux rig, whose clocksource is hpet because the boot-time
		// watchdog marked the TSC unstable, CLOCK_MONOTONIC costs 1931 ns and one
		// syscall per call against 5 ns and none for CLOCK_MONOTONIC_COARSE. Profiling a
		// churn workload there put four of the roughly nine clock reads per established
		// connection in this one function, which is the largest single group and the only
		// one that can be moved: the timer queue's reads cannot, because
		// pthread_cond_clockwait rejects both coarse clocks with EINVAL and its deadline
		// has to live in a clock the wait will accept.
		//
		// What is given up is precision this caller never had a use for. The coarse clock
		// advances once per tick, measured at 1.00 ms on that host, and the quantity it
		// decides is App's keep-alive timeout, which defaults to 30 000 ms. The
		// granularity is four orders of magnitude finer than the decision, and a timeout
		// that fires a millisecond late is not a different answer.
		//
		// It does impose a floor, which is worth stating because a test found it rather
		// than a reader. A timeout of the same order as the tick is no longer resolvable:
		// the store in touch() and the later subtraction can land on the same tick or on
		// adjacent ones, so an interval of about a millisecond reads as either zero or a
		// full tick depending on where the boundary falls. A 1 ms idle timeout under this
		// clock is therefore not a 1 ms timeout, and a test written to one is a coin
		// flip; it cost 2 failures in 200 runs before its timeout was raised clear of the
		// tick. Callers wanting sub-tick deadlines want TimerQueue, which is on
		// CLOCK_MONOTONIC for the unrelated reason above.
		//
		// Both sides of the comparison come through here, the store in touch() and the
		// subtraction in the expiry check, so they cannot end up on different clocks.
		// That is why this is the function that changed rather than touch().
		//
		// Elsewhere, and on any platform without a coarse monotonic clock, this is
		// steady_clock exactly as before: Windows and macOS have no CLOCK_MONOTONIC_COARSE,
		// and neither needs one, since their steady_clock is not paying for a disqualified
		// TSC.
		static std::int64_t now_ns()
		{
#if defined(__linux__) && defined(CLOCK_MONOTONIC_COARSE)
			::timespec ts{};
			if (::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0)
			{
				return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
			}
			// Falls through to steady_clock rather than returning a wrong answer. A
			// clock_gettime that fails on a clock the kernel advertises is not something
			// to paper over with a zero.
#endif
			return std::chrono::duration_cast<std::chrono::nanoseconds>(
			           std::chrono::steady_clock::now().time_since_epoch())
			    .count();
		}

		void touch() { state_->last_activity_ns.store(now_ns(), std::memory_order_relaxed); }

		// A read that failed after the timer closed the socket is reported as a timeout
		// rather than as whatever the backend makes of a closed descriptor. Without this
		// the caller takes the error branch and tries to write 400 to a socket that is
		// already gone, instead of the branch that says the client went quiet.
		ReadResult substitute(ReadResult result) const
		{
			if (!result && state_->expired.load(std::memory_order_relaxed))
			{
				return coroute::unexpected(Error::timeout());
			}
			return result;
		}

		// Re-arms itself rather than firing once per read. Cost is one scheduling call
		// per idle period per connection, and none at all for a connection that is busy
		// enough to keep resetting the clock.
		static void arm(IoContext& ctx, std::shared_ptr<State> state, std::chrono::milliseconds delay)
		{
			IoContext* ctx_ptr = &ctx;
			ctx.schedule(delay,
			             [ctx_ptr, state = std::move(state)]() mutable
			             {
				             const auto period =
				                 std::chrono::milliseconds(state->period_ms.load(std::memory_order_relaxed));

				             std::chrono::milliseconds remaining{0};
				             {
					             std::lock_guard<std::mutex> lock(state->mutex);
					             if (!state->inner || period <= std::chrono::milliseconds::zero())
					             {
						             return;  // the connection is gone, or the timeout was switched off
					             }

					             const auto quiet = std::chrono::duration_cast<std::chrono::milliseconds>(
					                 std::chrono::nanoseconds(now_ns() - state->last_activity_ns.load(
					                                                        std::memory_order_relaxed)));

					             if (quiet >= period)
					             {
						             // Set before closing, so the read this unblocks already
						             // sees the reason it was unblocked.
						             state->expired.store(true, std::memory_order_relaxed);
						             state->inner->close();
						             return;
					             }

					             remaining = period - quiet;
				             }

				             arm(*ctx_ptr, std::move(state), remaining);
			             });
		}

		std::unique_ptr<Connection> inner_;
		std::shared_ptr<State> state_;
	};

}  // namespace coroute::net
