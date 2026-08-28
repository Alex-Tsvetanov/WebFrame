#pragma once

#include "coroute/net/io_context.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace coroute::net
{

	// A one-shot action that runs unless it is cancelled first.
	//
	// It exists for the window between accepting a connection and knowing which protocol
	// is on it. A peer that connects and then says nothing leaves a coroutine parked in
	// the classification read, and nothing else in the stack will ever wake it: every
	// backend stores the timeout passed to Connection::set_timeout and none of them
	// enforces it.
	//
	// The action runs on a worker while the thing it acts on is owned by a coroutine
	// frame that may be running on another thread, so cancellation has to mean
	// "cancelled, and not running right now". A flag is not enough, because the frame
	// could be destroying the connection between the check and the call. The mutex is
	// what turns disarm() into a promise rather than a hint: the action either already
	// finished, or it will never start.
	//
	// Disarming is the destructor's job and nothing else's. The alternative, a call at
	// every exit from the window, is eleven call sites in App::serve_connection alone,
	// which is the same shape as the awaiter race described in the implementation
	// chapter: an interface whose ordinary use contains an easy mistake gets that
	// mistake made. So the window is a scope, and leaving the scope is the disarm.
	class Deadline
	{
	public:
		Deadline() = default;

		// A limit of zero arms nothing, which is how the deadline is switched off.
		Deadline(IoContext& ctx, std::chrono::milliseconds limit, std::function<void()> action)
		{
			if (limit <= std::chrono::milliseconds::zero() || !action)
			{
				return;
			}

			state_ = std::make_shared<State>();
			state_->action = std::move(action);

			ctx.schedule(limit,
			             [state = state_]
			             {
				             std::lock_guard<std::mutex> lock(state->mutex);
				             if (state->action)
				             {
					             state->action();
					             // One shot. Clearing it also tells replace() that the
					             // window is over rather than merely moved.
					             state->action = nullptr;
				             }
			             });
		}

		~Deadline() { disarm(); }

		Deadline(const Deadline&) = delete;
		Deadline& operator=(const Deadline&) = delete;
		Deadline(Deadline&&) = default;
		Deadline& operator=(Deadline&&) = default;

		// The object the action acts on has been wrapped or moved, so the action has to
		// be pointed at the new one. Does nothing if the deadline has already fired or
		// been disarmed, which is correct: an expired window does not reopen.
		void replace(std::function<void()> action)
		{
			if (!state_)
			{
				return;
			}
			std::lock_guard<std::mutex> lock(state_->mutex);
			if (state_->action)
			{
				state_->action = std::move(action);
			}
		}

		// Returns whether the deadline was still armed, which is only of interest to
		// tests and to counters. Safe to call more than once.
		bool disarm()
		{
			if (!state_)
			{
				return false;
			}
			std::lock_guard<std::mutex> lock(state_->mutex);
			const bool was_armed = static_cast<bool>(state_->action);
			state_->action = nullptr;
			return was_armed;
		}

	private:
		struct State
		{
			std::mutex mutex;
			std::function<void()> action;
		};

		// Shared with the scheduled callback, which outlives this object whenever the
		// connection is served faster than the limit, which is every healthy connection.
		std::shared_ptr<State> state_;
	};

}  // namespace coroute::net
