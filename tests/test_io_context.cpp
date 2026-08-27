#include <catch2/catch_test_macros.hpp>

#include <coroute/net/io_context.hpp>

#include <chrono>
#include <future>
#include <memory>
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
