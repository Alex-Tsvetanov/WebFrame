#include <catch2/catch_test_macros.hpp>
#include <coroute/net/deadline.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

using namespace coroute::net;
using namespace std::chrono_literals;

namespace
{
	// Only the two members Deadline uses. IoContext is a wide interface and a full mock
	// of it would be a page of `return {};`, none of which this exercises.
	class FakeContext : public IoContext
	{
	public:
		void run() override {}
		void run_one() override {}
		void stop() override {}
		bool stopped() const noexcept override { return false; }
		void post(std::function<void()> callback) override { callback(); }

		// Neither of these has anything to fake. This context exists to control when a
		// scheduled callback runs; it never accepts a connection, and saying so with a
		// nullptr is what IoContext already asks of a backend that cannot.
		const char* backend_name() const noexcept override { return "fake"; }
		std::unique_ptr<Listener> make_listener() override { return nullptr; }

		void schedule(std::chrono::milliseconds, std::function<void()> callback) override
		{
			// Held rather than run, so a test decides when the deadline expires. Real
			// timing belongs to the timer queue's own tests, not here.
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

	private:
		std::function<void()> pending_;
	};
}  // namespace

TEST_CASE("Deadline runs its action when it expires", "[deadline]")
{
	FakeContext ctx;
	bool closed = false;
	{
		Deadline d(ctx, 30s, [&] { closed = true; });
		REQUIRE(ctx.armed());
		ctx.fire();
	}
	CHECK(closed);
}

TEST_CASE("Leaving the scope disarms the deadline", "[deadline]")
{
	// The property the whole design rests on: the window is a scope, and no exit path
	// has to remember anything.
	FakeContext ctx;
	bool closed = false;
	{
		Deadline d(ctx, 30s, [&] { closed = true; });
	}
	ctx.fire();  // the timer still fires; the action must not
	CHECK_FALSE(closed);
}

TEST_CASE("Deadline follows the object it acts on", "[deadline]")
{
	// read_prefix and the TLS wrapper both hand back a different object, so closing the
	// one the deadline started with would close nothing anybody is reading from.
	FakeContext ctx;
	std::string closed;
	Deadline d(ctx, 30s, [&] { closed = "socket"; });
	d.replace([&] { closed = "wrapper"; });
	ctx.fire();
	CHECK(closed == "wrapper");
}

TEST_CASE("An expired window does not reopen", "[deadline]")
{
	FakeContext ctx;
	int fired = 0;
	Deadline d(ctx, 30s, [&] { ++fired; });
	ctx.fire();
	REQUIRE(fired == 1);

	d.replace([&] { ++fired; });
	ctx.fire();  // nothing left to fire, and replace must not have re-armed it
	CHECK(fired == 1);
}

TEST_CASE("A zero limit arms nothing", "[deadline]")
{
	FakeContext ctx;
	bool closed = false;
	Deadline d(ctx, 0ms, [&] { closed = true; });
	CHECK_FALSE(ctx.armed());
	ctx.fire();
	CHECK_FALSE(closed);
}

TEST_CASE("disarm reports whether it was still armed", "[deadline]")
{
	FakeContext ctx;
	Deadline d(ctx, 30s, [] {});
	CHECK(d.disarm());
	CHECK_FALSE(d.disarm());
}
