#include <catch2/catch_test_macros.hpp>
#include <coroute/coro/task.hpp>
#include <coroute/coro/cancellation.hpp>
#include <coroute/core/error.hpp>

#include <string>
#include <stdexcept>

using namespace coroute;

// ============================================================================
// Helper coroutines
// ============================================================================

static Task<int> return_42() { co_return 42; }

static Task<void> do_nothing() { co_return; }

static Task<std::string> return_string() { co_return "hello"; }

static Task<int> throw_runtime_error() {
	throw std::runtime_error("test error");
	co_return 0;  // unreachable
}

static Task<void> throw_void_runtime_error() {
	throw std::runtime_error("void error");
	co_return;
}

static Task<int> add(int a, int b) { co_return a + b; }

static Task<int> chain_tasks() {
	int a = co_await return_42();
	int b = co_await add(a, 8);
	co_return b;
}

static Task<void> yield_several_times() {
	co_await yield();
	co_await yield();
	co_await yield();
	co_return;
}

static Task<int> yield_then_return(int val) {
	co_await yield();
	co_return val;
}

// ============================================================================
// sync_wait tests
// ============================================================================

TEST_CASE("sync_wait returns value from Task<int>", "[task]") {
	auto t = return_42();
	REQUIRE(t.sync_wait() == 42);
}

TEST_CASE("sync_wait completes Task<void> without error", "[task]") {
	auto t = do_nothing();
	REQUIRE_NOTHROW(t.sync_wait());
}

TEST_CASE("sync_wait returns value from Task<string>", "[task]") {
	auto t = return_string();
	REQUIRE(t.sync_wait() == "hello");
}

TEST_CASE("sync_wait propagates exceptions from Task<int>", "[task]") {
	auto t = throw_runtime_error();
	REQUIRE_THROWS_AS(t.sync_wait(), std::runtime_error);
}

TEST_CASE("sync_wait propagates exceptions from Task<void>", "[task]") {
	auto t = throw_void_runtime_error();
	REQUIRE_THROWS_AS(t.sync_wait(), std::runtime_error);
}

TEST_CASE("sync_wait works with chained tasks", "[task]") {
	auto t = chain_tasks();
	REQUIRE(t.sync_wait() == 50);
}

// ============================================================================
// Task ownership and state tests
// ============================================================================

TEST_CASE("Task is valid after construction", "[task]") {
	auto t = return_42();
	REQUIRE(t.valid());
	REQUIRE(static_cast<bool>(t));
}

TEST_CASE("Default-constructed Task is invalid", "[task]") {
	Task<int> t;
	REQUIRE_FALSE(t.valid());
	REQUIRE_FALSE(static_cast<bool>(t));
}

TEST_CASE("Task move transfers ownership", "[task]") {
	auto t1 = return_42();
	REQUIRE(t1.valid());

	auto t2 = std::move(t1);
	REQUIRE_FALSE(t1.valid());
	REQUIRE(t2.valid());
	REQUIRE(t2.sync_wait() == 42);
}

TEST_CASE("Task move assignment transfers ownership", "[task]") {
	auto t1 = return_42();
	Task<int> t2;

	t2 = std::move(t1);
	REQUIRE_FALSE(t1.valid());
	REQUIRE(t2.valid());
	REQUIRE(t2.sync_wait() == 42);
}

// ============================================================================
// YieldAwaiter tests
// ============================================================================

TEST_CASE("yield suspends and resumes correctly", "[task]") {
	auto t = yield_several_times();
	REQUIRE_NOTHROW(t.sync_wait());
}

TEST_CASE("yield does not lose return value", "[task]") {
	auto t = yield_then_return(99);
	REQUIRE(t.sync_wait() == 99);
}

// ============================================================================
// start / start_detached / detach tests
// ============================================================================

TEST_CASE("start resumes a suspended task", "[task]") {
	auto t = return_42();
	REQUIRE_FALSE(t.done());
	t.start();
	REQUIRE(t.done());
}

TEST_CASE("start_detached runs fire-and-forget", "[task]") {
	bool executed = false;
	auto make_task = [&]() -> Task<void> {
		executed = true;
		co_return;
	};
	auto t = make_task();
	t.start_detached();
	// After start_detached, we no longer own the handle
	REQUIRE_FALSE(t.valid());
	REQUIRE(executed);
}

// ============================================================================
// CheckCancellationAwaiter tests
// ============================================================================

static Task<int> check_cancel_and_return() {
	// check_cancellation() inspects the promise's cancellation token
	co_await check_cancellation();
	co_return 100;
}

static Task<void> check_cancel_void() {
	co_await check_cancellation();
	co_return;
}

TEST_CASE("check_cancellation does not throw when not cancelled", "[task]") {
	CancellationSource source;
	auto t = check_cancel_and_return();
	t.set_cancellation_token(source.token());
	REQUIRE(t.sync_wait() == 100);
}

TEST_CASE("check_cancellation throws when cancelled", "[task]") {
	CancellationSource source;
	auto t = check_cancel_void();
	t.set_cancellation_token(source.token());
	source.cancel();
	REQUIRE_THROWS(t.sync_wait());
}

TEST_CASE("check_cancellation throws Error::cancelled()", "[task]") {
	CancellationSource source;
	source.cancel();  // Cancel before running

	auto t = check_cancel_void();
	t.set_cancellation_token(source.token());

	try {
		t.sync_wait();
		FAIL("Expected Error to be thrown");
	} catch (const Error& e) {
		REQUIRE(e.is_cancelled());
	}
}

// ============================================================================
// TaskResult alias tests
// ============================================================================

static Task<Result<int>> return_result_ok() { co_return 42; }

static Task<Result<int>> return_result_error() {
	co_return coroute::unexpected(Error::http(HttpError::NotFound, "not found"));
}

TEST_CASE("TaskResult with successful value", "[task]") {
	auto t = return_result_ok();
	auto result = t.sync_wait();
	REQUIRE(result.has_value());
	REQUIRE(*result == 42);
}

TEST_CASE("TaskResult with error value", "[task]") {
	auto t = return_result_error();
	auto result = t.sync_wait();
	REQUIRE_FALSE(result.has_value());
	REQUIRE(result.error().is_http());
	REQUIRE(result.error().http_status() == 404);
}
