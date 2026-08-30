#pragma once

#include "coroute/net/io_context.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
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

		Task<WriteResult> async_write_zero_copy(const void* buffer, size_t len) override
		{
			touch();
			co_return co_await inner_->async_write_zero_copy(buffer, len);
		}

		[[nodiscard]] bool supports_zero_copy_send() const noexcept override
		{
			return inner_->supports_zero_copy_send();
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

		static std::int64_t now_ns()
		{
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
