#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "coroute/coro/task.hpp"

namespace coroute
{

	// ============================================================================
	// Deferred<T>: a value the page does not have to wait for
	// ============================================================================
	//
	// A view handler can fetch its data two ways. It can await the call, in which case
	// the value is in the model before anything is rendered and the page arrives
	// complete. Or it can hand the model a Deferred<T>, in which case the page is sent
	// immediately with a hole where the value goes, and the value is streamed into that
	// hole when it arrives.
	//
	// The second is worth having when one slow query would otherwise hold up an
	// otherwise fast page. It is not free: the response has to stay open, and the
	// client needs a way to understand a value that is not there yet.
	//
	// That second half is the interesting one. The placeholder emitted here is not a
	// marker for a DOM swap, it is the server side of a Promise: Deferred<T> in C++ has
	// a counterpart on the other side of the wire that page code can await, compose
	// with Promise.all, and attach error handling to. Early flushing on its own is old
	// news, BigPipe did it in 2010. A value whose pending-ness is expressed in the type
	// system on both ends is not.
	//
	// The work starts as soon as the Deferred is constructed, not when it is first
	// looked at. Several deferred fields in one model should overlap each other and the
	// render rather than run in sequence, and lazy evaluation would serialise exactly
	// the thing this exists to parallelise.

	// The part of a Deferred that does not depend on T.
	//
	// Serialisation has to record which fields are still pending without knowing what
	// they hold, so this is what the collector below stores.
	class DeferredState
	{
	public:
		virtual ~DeferredState() = default;

		[[nodiscard]] virtual bool ready() const noexcept = 0;

		// Valid only once ready() is true.
		[[nodiscard]] virtual nlohmann::json to_json() const = 0;

		// Invoked when the value arrives, or immediately if it already has. Used by the
		// renderer to flush a chunk the moment a slot can be filled.
		virtual void on_ready(std::function<void()> callback) = 0;
	};

	// Gathers the deferreds met while a model is being serialised.
	//
	// The alternative is making every ViewModel declare its deferred fields, which
	// duplicates what the JSON conversion already walks and rots the moment someone
	// adds a field and forgets. Collecting during serialisation means a Deferred is
	// registered exactly when its placeholder is written, so the two cannot disagree.
	class DeferredCollector
	{
	public:
		// Makes this collector the active one for the current thread, and restores the
		// previous one on the way out. Scoped rather than set-and-clear so an exception
		// mid-render cannot leave a dangling collector behind.
		class Scope
		{
		public:
			explicit Scope(DeferredCollector& collector) noexcept : previous_(active_)
			{
				active_ = &collector;
			}
			~Scope() { active_ = previous_; }

			Scope(const Scope&) = delete;
			Scope& operator=(const Scope&) = delete;

		private:
			DeferredCollector* previous_;
		};

		[[nodiscard]] static DeferredCollector* active() noexcept { return active_; }

		// Returns the slot id the placeholder should carry.
		std::size_t add(std::shared_ptr<DeferredState> state)
		{
			pending_.push_back(std::move(state));
			return pending_.size() - 1;
		}

		[[nodiscard]] const std::vector<std::shared_ptr<DeferredState>>& pending() const noexcept
		{
			return pending_;
		}

		[[nodiscard]] bool empty() const noexcept { return pending_.empty(); }

	private:
		std::vector<std::shared_ptr<DeferredState>> pending_;

		// Thread-local because a render belongs to one thread for its whole duration,
		// and a shared collector would mix slots from concurrent requests.
		static inline thread_local DeferredCollector* active_ = nullptr;
	};

	// The key a placeholder is written under. Distinctive enough that a real field
	// called this would be a deliberate act.
	inline constexpr const char* deferred_slot_key = "__coroute_deferred";

	template <typename T>
	class Deferred
	{
		struct State : DeferredState
		{
			mutable std::mutex mutex;
			std::optional<T> value;
			std::vector<std::function<void()>> waiters;
			std::atomic<bool> done{false};

			[[nodiscard]] bool ready() const noexcept override { return done.load(std::memory_order_acquire); }

			[[nodiscard]] nlohmann::json to_json() const override
			{
				std::lock_guard lock(mutex);
				return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
			}

			void on_ready(std::function<void()> callback) override
			{
				{
					std::lock_guard lock(mutex);
					if (!done.load(std::memory_order_acquire))
					{
						waiters.push_back(std::move(callback));
						return;
					}
				}
				// Already resolved. Called here rather than dropped, so a caller does not
				// have to check readiness before registering and race with the answer
				// arriving in between.
				callback();
			}

			void resolve(T result)
			{
				std::vector<std::function<void()>> to_call;
				{
					std::lock_guard lock(mutex);
					value = std::move(result);
					done.store(true, std::memory_order_release);
					to_call.swap(waiters);
				}
				// Outside the lock: a waiter is free to do anything, including touching
				// this Deferred again.
				for (auto& callback : to_call)
				{
					callback();
				}
			}
		};

	public:
		using value_type = T;

		// An empty Deferred is permanently pending. Useful only as a default member,
		// which is why it exists at all.
		Deferred() : state_(std::make_shared<State>()) { }

		// Already-resolved. Costs nothing at render time and keeps calling code uniform
		// whether a value came from cache or from a query.
		explicit Deferred(T value) : state_(std::make_shared<State>()) { state_->resolve(std::move(value)); }

		// Starts immediately. See the note above on why this is not lazy.
		explicit Deferred(Task<T> task) : state_(std::make_shared<State>())
		{
			start(state_, std::move(task)).start_detached();
		}

		[[nodiscard]] bool ready() const noexcept { return state_->ready(); }

		// Valid once ready(). Returns nothing while still pending rather than blocking:
		// a view that wants to wait should await the call instead of deferring it.
		[[nodiscard]] const T* value() const noexcept
		{
			std::lock_guard lock(state_->mutex);
			return state_->value ? &*state_->value : nullptr;
		}

		[[nodiscard]] std::shared_ptr<DeferredState> state() const noexcept { return state_; }

	private:
		static Task<void> start(std::shared_ptr<State> state, Task<T> task)
		{
			// The state is captured by value so the work outlives the Deferred that
			// started it. A handler that returns before its data arrives is the normal
			// case here, not an edge one.
			state->resolve(co_await std::move(task));
		}

		std::shared_ptr<State> state_;
	};

	// Serialises a Deferred into the model's JSON.
	//
	// Three cases, and the distinction matters:
	//
	//   resolved             emit the value, because there is nothing to defer
	//   pending, collecting  emit a placeholder and register the slot
	//   pending, not         emit null
	//
	// The last one is not an oversight. Without a collector nobody is streaming, so a
	// placeholder would be a promise the page never keeps, and a hole that stays empty
	// forever is worse than an honest null.
	template <typename T>
	void to_json(nlohmann::json& json, const Deferred<T>& deferred)
	{
		if (deferred.ready())
		{
			json = deferred.state()->to_json();
			return;
		}

		DeferredCollector* collector = DeferredCollector::active();
		if (collector == nullptr)
		{
			json = nullptr;
			return;
		}

		json = nlohmann::json::object();
		json[deferred_slot_key] = collector->add(deferred.state());
	}

}  // namespace coroute
