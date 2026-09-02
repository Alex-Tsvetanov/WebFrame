#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <coroute/net/io_context.hpp>

#include "io_backend_arms.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <set>
#include <thread>
#include <vector>

using namespace coroute;
using namespace coroute::net;
using namespace std::chrono_literals;

namespace
{

	// Runs ctx->run() on another thread and reports whether it came back within the
	// deadline. On failure the thread is detached rather than joined, and the context
	// is held by shared_ptr so the detached thread cannot outlive it: a hung loop
	// should fail the test, not deadlock the whole suite.
	bool run_then(const std::shared_ptr<IoContext>& ctx, const std::function<void()>& action,
	              std::chrono::milliseconds deadline = 5s)
	{
		std::promise<void> finished;
		auto future = finished.get_future();

		std::thread loop(
			[ctx, &finished]
			{
				ctx->run();
				finished.set_value();
			});

		action();

		const bool done = future.wait_for(deadline) == std::future_status::ready;
		if (done)
		{
			loop.join();
		}
		else
		{
			loop.detach();
		}
		return done;
	}

}  // namespace

TEST_CASE("the event loop stops when asked", "[io_context]")
{
	const IoBackend backend = GENERATE(from_range(coroute::testing::io_backend_arms()));
	INFO("backend " << io_backend_name(backend));

	SECTION("stop() after the loop is running")
	{
		auto ctx = std::shared_ptr<IoContext>(coroute::testing::context_or_skip(1, backend));
		REQUIRE(run_then(ctx,
		                 [&]
		                 {
							 std::this_thread::sleep_for(200ms);  // let run() get going
							 ctx->stop();
						 }));
		REQUIRE(ctx->stopped());
	}

	SECTION("stop() racing the start of run()")
	{
		// The loop is started and stopped immediately, so stop() may well land before
		// run() has been scheduled at all.
		//
		// Every backend used to clear the stopped flag at the top of run(), which
		// erased a stop that arrived first and left the workers spinning forever. It
		// is not a theoretical race: it hung a test for ten minutes, and App::stop()
		// called soon after App::run() would hit exactly the same thing.
		auto ctx = std::shared_ptr<IoContext>(coroute::testing::context_or_skip(2, backend));
		REQUIRE(run_then(ctx, [&] { ctx->stop(); }));
		REQUIRE(ctx->stopped());
	}

	SECTION("stop() before run() is ever called")
	{
		auto ctx = std::shared_ptr<IoContext>(coroute::testing::context_or_skip(1, backend));
		ctx->stop();
		REQUIRE(ctx->stopped());
		REQUIRE(run_then(ctx, [] { }));
	}

	SECTION("stop() is idempotent")
	{
		auto ctx = std::shared_ptr<IoContext>(coroute::testing::context_or_skip(1, backend));
		REQUIRE(run_then(ctx,
		                 [&]
		                 {
							 ctx->stop();
							 ctx->stop();
							 ctx->stop();
						 }));
		REQUIRE(ctx->stopped());
	}
}

TEST_CASE("posted work reaches an idle worker without waiting out the poll timeout",
          "[io_context]")
{
	// The regression this exists for: io_uring's wake() wrote to an eventfd that nothing
	// read or registered with the ring, so it woke nothing. epoll's post() did not even
	// try, relying on epoll_wait's 100ms timeout to notice the queue. Completions still
	// arrived on their own, which is why the suite stayed green, but work posted from
	// another thread waited for the poll to expire on its own schedule: measured at 50ms
	// on epoll, and up to the full timeout on io_uring. TimerQueue routes every idle
	// timeout and handshake deadline through post(), and a wait that blocks rather than
	// times out would never deliver it at all.
	//
	// Measured over repeated posts, and the median taken, because a single post cannot
	// tell the two cases apart. On a machine whose cores have gone idle, waking one
	// costs several hundred microseconds by itself -- an effect this project measured
	// separately at around 490us -- which is the same order as the io_uring timeout this
	// is supposed to be detecting. The first post pays that; the rest do not, because
	// the core is awake by then. So the median over a run isolates the ring's wake
	// latency from the CPU's, and it is the ring's that this test is about.
	//
	// The 2ms gap is chosen to be longer than io_uring's 1ms timeout, so each post finds
	// the worker freshly parked in a wait rather than still spinning through the
	// previous one, and short enough that the core does not go back to a deep idle
	// state between posts.
	//
	// The ring carries no I/O throughout. With traffic on it a completion would end the
	// wait and a post would ride along on someone else's wakeup, which is exactly the
	// case that hid this for as long as it was hidden.
	const IoBackend backend = GENERATE(from_range(coroute::testing::io_backend_arms()));
	INFO("backend " << io_backend_name(backend));

	auto ctx = std::shared_ptr<IoContext>(coroute::testing::context_or_skip(2, backend));

	std::thread loop([ctx] { ctx->run(); });
	std::this_thread::sleep_for(50ms);

	constexpr int posts = 21;
	std::vector<std::int64_t> latencies_us;
	latencies_us.reserve(posts);

	bool all_arrived = true;
	for (int i = 0; i < posts; ++i)
	{
		std::promise<std::chrono::steady_clock::duration> delivered;
		auto took = delivered.get_future();

		const auto posted_at = std::chrono::steady_clock::now();
		ctx->post([&delivered, posted_at]
		          { delivered.set_value(std::chrono::steady_clock::now() - posted_at); });

		if (took.wait_for(2s) != std::future_status::ready)
		{
			all_arrived = false;
			break;
		}
		latencies_us.push_back(
			std::chrono::duration_cast<std::chrono::microseconds>(took.get()).count());
		std::this_thread::sleep_for(2ms);
	}

	ctx->stop();
	loop.join();

	REQUIRE(all_arrived);
	REQUIRE(latencies_us.size() == posts);

	std::sort(latencies_us.begin(), latencies_us.end());
	const std::int64_t median = latencies_us[latencies_us.size() / 2];
	INFO("median delivery " << median << "us, first " << latencies_us.front() << "us, worst "
	                        << latencies_us.back() << "us");

	// A fifth of io_uring's 1ms timeout and a fiftieth of epoll's 100ms one, so neither
	// backend can pass by waiting one out.
	CHECK(median < 200);
}

TEST_CASE("worker_count reports what run() will spawn", "[io_context]")
{
	// Multi-accept sizes its pool from this, so a backend reporting 1 while running N
	// would quietly under-provision the accept path.
	const IoBackend backend = GENERATE(from_range(coroute::testing::io_backend_arms()));
	INFO("backend " << io_backend_name(backend));

	for (size_t requested : {size_t{1}, size_t{2}, size_t{4}})
	{
		auto ctx = coroute::testing::context_or_skip(requested, backend);
		INFO("requested " << requested);
		REQUIRE(ctx->worker_count() == requested);
	}
}

TEST_CASE("work can be directed at a specific worker thread", "[io_context][affinity]")
{
	// The property QUIC connection sharding is built on. A connection's state is owned
	// by exactly one thread, because ngtcp2 is not reentrant, so a packet that the
	// kernel delivered to the wrong worker has to be handed to the thread that owns it
	// and not merely to whichever worker is free.

	constexpr size_t workers = 4;
	const IoBackend backend = GENERATE(from_range(coroute::testing::io_backend_arms()));
	INFO("backend " << io_backend_name(backend));

	auto ctx = std::shared_ptr<IoContext>(coroute::testing::context_or_skip(workers, backend));
	REQUIRE(ctx);

	if (!ctx->supports_worker_affinity())
	{
		// Every worker on this backend pulls from one shared queue, so no thread has an
		// identity to target. Saying so is the useful outcome: the caller then knows to
		// keep QUIC on a single thread rather than discovering the race later.
		SUCCEED("this backend does not support worker affinity");
		return;
	}

	REQUIRE(ctx->worker_count() == workers);

	// Two rounds per worker, so the test can tell "landed on some thread" apart from
	// "landed on the same thread both times", which is the property that actually
	// matters.
	constexpr size_t rounds = 2;
	std::array<std::array<std::thread::id, rounds>, workers> observed{};
	std::atomic<size_t> completed{0};

	const bool finished = run_then(ctx,
	                               [&]
	                               {
									   for (size_t round = 0; round < rounds; ++round)
									   {
										   for (size_t worker = 0; worker < workers; ++worker)
										   {
											   ctx->run_on_worker(worker,
			                                                      [&, worker, round]
			                                                      {
																	  observed[worker][round] =
																		  std::this_thread::get_id();
																	  completed.fetch_add(1);
																  });
										   }
									   }

									   // Spin rather than sleep a fixed amount: a fixed wait is either
									   // flaky on a loaded machine or slow on an idle one.
									   const auto deadline = std::chrono::steady_clock::now() + 5s;
									   while (completed.load() < workers * rounds &&
			                                  std::chrono::steady_clock::now() < deadline)
									   {
										   std::this_thread::yield();
									   }
									   ctx->stop();
								   });

	REQUIRE(finished);
	REQUIRE(completed.load() == workers * rounds);

	SECTION("each worker keeps its identity across calls")
	{
		for (size_t worker = 0; worker < workers; ++worker)
		{
			INFO("worker " << worker);
			REQUIRE(observed[worker][0] != std::thread::id{});
			REQUIRE(observed[worker][0] == observed[worker][1]);
		}
	}

	SECTION("different indices reach different threads")
	{
		// Without this the whole scheme collapses: one thread answering for every index
		// would pass the check above and still serialise the entire server.
		std::set<std::thread::id> distinct;
		for (size_t worker = 0; worker < workers; ++worker)
		{
			distinct.insert(observed[worker][0]);
		}
		REQUIRE(distinct.size() == workers);
	}
}
