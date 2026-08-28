#include <catch2/catch_test_macros.hpp>
#include <coroute/net/timer_queue.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

using namespace coroute::net;
using namespace std::chrono_literals;

namespace
{
	// Runs callbacks on the calling thread, which here is the timer thread. That is not
	// how a backend uses it, since a real post() hands the callback to a worker, but it
	// keeps the test to two threads and tests the queue rather than a loop.
	struct Collector
	{
		std::mutex m;
		std::condition_variable cv;
		std::vector<std::string> seen;

		TimerQueue::PostFn post()
		{
			return [this](std::function<void()> cb) { cb(); };
		}

		void record(std::string what)
		{
			{
				std::lock_guard<std::mutex> lock(m);
				seen.push_back(std::move(what));
			}
			cv.notify_all();
		}

		// Waits for a count rather than sleeping for a duration, so the test does not
		// fail on a loaded machine and does not pass by luck on an idle one.
		bool wait_for_count(size_t n, std::chrono::milliseconds limit)
		{
			std::unique_lock<std::mutex> lock(m);
			return cv.wait_for(lock, limit, [&] { return seen.size() >= n; });
		}

		std::vector<std::string> snapshot()
		{
			std::lock_guard<std::mutex> lock(m);
			return seen;
		}
	};
}  // namespace

TEST_CASE("TimerQueue fires in deadline order", "[timer]")
{
	Collector c;
	TimerQueue timers(c.post());

	// Scheduled in the reverse of the order they must fire in, so a queue that ignored
	// the deadline and ran them first-in-first-out would produce "late, early".
	timers.schedule(120ms, [&] { c.record("late"); });
	timers.schedule(10ms, [&] { c.record("early"); });

	REQUIRE(c.wait_for_count(2, 3s));
	CHECK(c.snapshot() == std::vector<std::string>{"early", "late"});
}

TEST_CASE("TimerQueue drops what is still pending at stop", "[timer]")
{
	Collector c;
	{
		TimerQueue timers(c.post());
		timers.schedule(10ms, [&] { c.record("due"); });
		REQUIRE(c.wait_for_count(1, 3s));

		timers.schedule(1h, [&] { c.record("never"); });
		// Destructor stops the queue. The hour-long timer is dropped rather than run,
		// and, the part that matters, the destructor does not wait an hour for it.
	}
	CHECK(c.snapshot() == std::vector<std::string>{"due"});
}

TEST_CASE("A callback can schedule another timer", "[timer]")
{
	// The queue runs callbacks with its mutex released. Holding it would deadlock here,
	// and this is the case that catches that: the callback re-enters schedule().
	Collector c;
	TimerQueue timers(c.post());

	timers.schedule(10ms,
	                [&]
	                {
		                c.record("first");
		                timers.schedule(10ms, [&] { c.record("second"); });
	                });

	REQUIRE(c.wait_for_count(2, 3s));
	CHECK(c.snapshot() == std::vector<std::string>{"first", "second"});
}

TEST_CASE("TimerQueue with nothing scheduled starts no thread", "[timer]")
{
	// Not observable directly without reaching into the class, so this asserts the
	// consequence instead: construction and destruction are free and do not block.
	std::atomic<bool> called{false};
	{
		TimerQueue timers([&](std::function<void()> cb) { called = true; cb(); });
	}
	CHECK_FALSE(called.load());
}
