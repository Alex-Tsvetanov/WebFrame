#include <catch2/catch_test_macros.hpp>

#include <coroute/net/io_context.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <set>
#include <thread>

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
	SECTION("stop() after the loop is running")
	{
		auto ctx = std::shared_ptr<IoContext>(IoContext::create(1));
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
		auto ctx = std::shared_ptr<IoContext>(IoContext::create(2));
		REQUIRE(run_then(ctx, [&] { ctx->stop(); }));
		REQUIRE(ctx->stopped());
	}

	SECTION("stop() before run() is ever called")
	{
		auto ctx = std::shared_ptr<IoContext>(IoContext::create(1));
		ctx->stop();
		REQUIRE(ctx->stopped());
		REQUIRE(run_then(ctx, [] { }));
	}

	SECTION("stop() is idempotent")
	{
		auto ctx = std::shared_ptr<IoContext>(IoContext::create(1));
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

TEST_CASE("worker_count reports what run() will spawn", "[io_context]")
{
	// Multi-accept sizes its pool from this, so a backend reporting 1 while running N
	// would quietly under-provision the accept path.
	for (size_t requested : {size_t{1}, size_t{2}, size_t{4}})
	{
		auto ctx = IoContext::create(requested);
		INFO("requested " << requested);
		REQUIRE(ctx->worker_count() == requested);
	}
}

#if defined(COROUTE_PLATFORM_LINUX)
TEST_CASE("Linux epoll and io_uring share one binary", "[io_context][backend]")
{
	// The whole point of compiling both: a runtime flag, not a rebuild. If either
	// create() throws, the comparison arms cannot come from the same binary.
	auto epoll = IoContext::create(1, IoBackend::Epoll);
	REQUIRE(epoll);
	REQUIRE(epoll->backend_name() == "epoll");
	epoll->stop();

	auto uring = IoContext::create(1, IoBackend::IoUring);
	REQUIRE(uring);
	REQUIRE(uring->backend_name() == "io_uring");
	uring->stop();

	IoBackend parsed = IoBackend::Default;
	REQUIRE(parse_io_backend("epoll", parsed));
	REQUIRE(parsed == IoBackend::Epoll);
	REQUIRE(parse_io_backend("io_uring", parsed));
	REQUIRE(parsed == IoBackend::IoUring);
}
#endif

TEST_CASE("work can be directed at a specific worker thread", "[io_context][affinity]")
{
	// The property QUIC connection sharding is built on. A connection's state is owned
	// by exactly one thread, because ngtcp2 is not reentrant, so a packet that the
	// kernel delivered to the wrong worker has to be handed to the thread that owns it
	// and not merely to whichever worker is free.

	constexpr size_t workers = 4;
	auto ctx = std::shared_ptr<IoContext>(IoContext::create(workers));
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
