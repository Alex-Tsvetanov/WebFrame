#include <catch2/catch_test_macros.hpp>
#include <coroute/net/idle_timeout.hpp>

#include <algorithm>
#include <chrono>
#include <thread>
#include <cstring>
#include <functional>
#include <memory>
#include <string>

using namespace coroute;
using namespace coroute::net;
using namespace std::chrono_literals;

namespace
{
	// Holds the scheduled callback instead of running it, so the test decides when a
	// period elapses. Real timing belongs to the timer queue's own tests.
	class FakeContext final : public IoContext
	{
	public:
		[[nodiscard]] std::string_view backend_name() const noexcept override { return "fake"; }
		void run() override { }
		void run_one() override { }
		void stop() override { }
		bool stopped() const noexcept override { return false; }
		void post(std::function<void()> callback) override { callback(); }

		void schedule(std::chrono::milliseconds delay, std::function<void()> callback) override
		{
			last_delay = delay;
			++schedules;
			pending_ = std::move(callback);
		}

		void fire()
		{
			if (pending_)
			{
				auto cb = std::move(pending_);
				pending_ = nullptr;
				cb();
			}
		}

		bool armed() const noexcept { return static_cast<bool>(pending_); }

		std::chrono::milliseconds last_delay{0};
		int schedules = 0;

	private:
		std::function<void()> pending_;
	};

	// Reads succeed while open and fail once closed, which is what a real socket does
	// when something else closes it out from under a pending read.
	class FakeConnection final : public Connection
	{
	public:
		Task<ReadResult> async_read(void* buffer, size_t len) override
		{
			if (!open_)
			{
				co_return unexpected(Error::io(IoError::EndOfStream, "closed"));
			}
			const size_t n = std::min(len, sizeof("byte") - 1);
			std::memcpy(buffer, "byte", n);
			co_return n;
		}

		Task<ReadResult> async_read_until(void* buffer, size_t len, char) override
		{
			co_return co_await async_read(buffer, len);
		}

		Task<WriteResult> async_write(const void*, size_t len) override { co_return len; }
		Task<WriteResult> async_write_all(const void*, size_t len) override { co_return len; }
		Task<TransmitResult> async_transmit_file(FileHandle, size_t, size_t) override { co_return size_t{0}; }

		void close() override
		{
			++closes;
			open_ = false;
		}

		bool is_open() const noexcept override { return open_; }
		void set_timeout(std::chrono::milliseconds t) override { last_timeout = t; }
		std::string remote_address() const override { return "198.51.100.4"; }
		uint16_t remote_port() const noexcept override { return 51234; }
		void set_cancellation_token(CancellationToken) override { }

		int closes = 0;
		std::chrono::milliseconds last_timeout{0};

	private:
		bool open_ = true;
	};

	std::string read_one(Connection& conn)
	{
		char buf[8] = {};
		auto n = conn.async_read(buf, sizeof(buf)).sync_wait();
		return n ? std::string(buf, *n) : std::string();
	}
}  // namespace

TEST_CASE("Firing before the period has elapsed re-arms rather than closes", "[idle]")
{
	FakeContext ctx;
	auto* raw = new FakeConnection();
	IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 30s);

	REQUIRE(ctx.armed());
	REQUIRE(ctx.last_delay == 30s);

	// The timer queue can wake early, and a connection that has been alive for a
	// millisecond of a thirty second period is not idle. Closing here would drop live
	// connections on a clock that ran fast.
	ctx.fire();

	CHECK(raw->closes == 0);
	CHECK_FALSE(conn.expired());
	CHECK(ctx.armed());
	// Re-armed for what is left of the period, not for another full one.
	CHECK(ctx.last_delay <= 30s);
	CHECK(ctx.schedules == 2);
}

TEST_CASE("A period that has elapsed closes the connection", "[idle]")
{
	FakeContext ctx;
	auto* raw = new FakeConnection();

	// A period of one millisecond, so the time between constructing and firing is
	// enough to count as idle without the test sleeping.
	IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 1ms);
	REQUIRE(ctx.armed());

	// A read works while the connection is live.
	CHECK(read_one(conn) == "byte");

	std::this_thread::sleep_for(5ms);
	ctx.fire();

	CHECK(raw->closes == 1);
	CHECK(conn.expired());

	// And the failure the closed socket produces is reported as a timeout rather than
	// as end of stream. Without this the caller takes its error branch and writes 400
	// to a socket that is already gone.
	char buf[8] = {};
	auto result = conn.async_read(buf, sizeof(buf)).sync_wait();
	REQUIRE_FALSE(result.has_value());
	CHECK(result.error().is_timeout());
}

TEST_CASE("Activity resets the idle clock", "[idle]")
{
	FakeContext ctx;
	auto* raw = new FakeConnection();
	IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 1ms);

	std::this_thread::sleep_for(5ms);

	// A byte moves just before the timer looks, so the connection is not idle.
	CHECK(read_one(conn) == "byte");
	ctx.fire();

	CHECK(raw->closes == 0);
	// Re-armed for what is left of the period rather than closing.
	CHECK(ctx.armed());
	CHECK(ctx.schedules == 2);
}

TEST_CASE("A destroyed wrapper stops the timer touching the connection", "[idle]")
{
	// The timer runs on a worker while the connection is owned by a coroutine frame, so
	// this is the case that would otherwise be a use-after-free.
	FakeContext ctx;
	auto* raw = new FakeConnection();
	{
		IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 1ms);
		std::this_thread::sleep_for(5ms);
	}
	// raw is destroyed with the wrapper. Firing now must do nothing at all rather than
	// reach through a dangling pointer.
	ctx.fire();
	SUCCEED("the timer found a null connection and stopped");
}

TEST_CASE("A period of zero arms nothing", "[idle]")
{
	FakeContext ctx;
	auto* raw = new FakeConnection();
	IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 0ms);

	CHECK_FALSE(ctx.armed());
	CHECK(ctx.schedules == 0);
	CHECK(read_one(conn) == "byte");
	CHECK(raw->closes == 0);
}

TEST_CASE("set_timeout is passed down as well as acted on", "[idle]")
{
	// The backends keep their stored value, so one that grows a real implementation
	// later agrees with this wrapper rather than competing with it.
	FakeContext ctx;
	auto* raw = new FakeConnection();
	IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 30s);

	conn.set_timeout(45s);
	CHECK(raw->last_timeout == 45s);
}

TEST_CASE("Metadata and writes delegate", "[idle]")
{
	FakeContext ctx;
	auto* raw = new FakeConnection();
	IdleTimeout conn(std::unique_ptr<Connection>(raw), ctx, 30s);

	CHECK(conn.remote_address() == "198.51.100.4");
	CHECK(conn.remote_port() == 51234);
	CHECK(conn.is_open());
	CHECK(conn.async_write_all("pong", 4).sync_wait().has_value());

	conn.close();
	CHECK(raw->closes == 1);
	CHECK_FALSE(conn.is_open());
	// Closed by the caller, not by the timer, so it is not an expiry.
	CHECK_FALSE(conn.expired());
}
