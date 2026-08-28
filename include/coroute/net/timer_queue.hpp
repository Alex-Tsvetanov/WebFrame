#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace coroute::net
{

	// A deadline queue for IoContext::schedule.
	//
	// It exists because three of the four backends implemented schedule() by spawning a
	// detached thread that slept for the delay and then posted. That is fine for the
	// handful of one-off timers the interface was written for, and ruinous for anything
	// per-connection: a watchdog on the classification read would have cost one sleeping
	// thread per accepted connection. The IOCP backend already had a real queue, so this
	// is that queue lifted out rather than a new design.
	//
	// One thread, started on first use, so a context that never schedules anything never
	// pays for it. Callbacks are handed to `post`, meaning they run on a worker like every
	// other callback and the timer thread never touches the loop's own state.
	//
	// Deliberately steady_clock and not system_clock, which is what the IOCP version used.
	// A timer must not move when the wall clock is stepped by NTP or by a user; a ten
	// second handshake deadline that a clock correction turns into ten minutes is a
	// slowloris window that appears out of nowhere and cannot be reproduced.
	//
	// Callbacks still queued at stop() are dropped, not run. A callback that must happen
	// on shutdown does not belong on a timer.
	class TimerQueue
	{
	public:
		using PostFn = std::function<void(std::function<void()>)>;

		explicit TimerQueue(PostFn post) : post_(std::move(post)) {}

		~TimerQueue() { stop(); }

		TimerQueue(const TimerQueue&) = delete;
		TimerQueue& operator=(const TimerQueue&) = delete;

		void schedule(std::chrono::milliseconds delay, std::function<void()> callback)
		{
			{
				std::lock_guard lock(mutex_);
				if (stopped_)
				{
					return;
				}
				queue_.push({std::chrono::steady_clock::now() + delay, std::move(callback)});

				// Started here rather than in the constructor. Holding the mutex while the
				// new thread's first act is to take it is safe: it simply waits.
				if (!thread_.joinable())
				{
					thread_ = std::thread([this] { run(); });
				}
			}
			cv_.notify_all();
		}

		// Safe to call more than once, and called by the destructor. Must not be called
		// from a scheduled callback, which runs on a worker rather than on this thread.
		void stop()
		{
			{
				std::lock_guard lock(mutex_);
				if (stopped_)
				{
					return;
				}
				stopped_ = true;
			}
			cv_.notify_all();
			if (thread_.joinable())
			{
				thread_.join();
			}
		}

	private:
		struct Entry
		{
			std::chrono::steady_clock::time_point expiry;
			std::function<void()> callback;

			bool operator>(const Entry& other) const { return expiry > other.expiry; }
		};

		void run()
		{
			std::unique_lock lock(mutex_);
			while (!stopped_)
			{
				if (queue_.empty())
				{
					cv_.wait(lock);
					continue;
				}

				const auto expiry = queue_.top().expiry;
				if (expiry > std::chrono::steady_clock::now())
				{
					cv_.wait_until(lock, expiry);
					continue;
				}

				// const_cast because priority_queue only ever hands out a const top, and
				// the callback is about to be popped anyway.
				auto callback = std::move(const_cast<Entry&>(queue_.top()).callback);
				queue_.pop();

				// Unlocked across post: a callback that schedules another timer would
				// otherwise deadlock on this mutex.
				lock.unlock();
				post_(std::move(callback));
				lock.lock();
			}
		}

		PostFn post_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue_;
		std::thread thread_;
		bool stopped_ = false;
	};

}  // namespace coroute::net
