// A page with a value that has not arrived yet.
//
// Phase 7 reduced to something a browser could load: the shell goes out immediately,
// the slow field arrives later, and page code awaits it as a Promise rather than
// watching for a DOM mutation.
#include <chrono>
#include <coroutine>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "coroute/core/app.hpp"

using namespace coroute;

namespace
{

	struct Dashboard
	{
		std::string title;
		int visitors_now = 0;
		Deferred<std::string> slow_report;
	};

	void to_json(nlohmann::json& json, const Dashboard& model)
	{
		json = nlohmann::json{
			{       "title",        model.title},
	        {"visitors_now", model.visitors_now},
	        { "slow_report",  model.slow_report}
        };
	}

	// A delay that does not block the event loop.
	//
	// Sleeping on the calling thread would stall the whole loop and delay the shell
	// too, which would make the deferred page indistinguishable from the awaited one
	// and hide the exact thing this demonstrates. A thread is crude but this is a test
	// fixture standing in for a slow query, not a scheduler.
	struct SleepAwaiter
	{
		std::chrono::milliseconds delay;

		[[nodiscard]] bool await_ready() const noexcept { return delay.count() <= 0; }

		void await_suspend(std::coroutine_handle<> handle) const
		{
			std::thread(
				[handle, wait = delay]
				{
					std::this_thread::sleep_for(wait);
					handle.resume();
				})
				.detach();
		}

		void await_resume() const noexcept { }
	};

	Task<std::string> slow_value(std::chrono::milliseconds delay)
	{
		co_await SleepAwaiter{delay};
		co_return std::string("the slow part");
	}

}  // namespace

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::fprintf(stderr, "usage: deferred_app <port> [delay_ms] [templates_dir]\n");
		return 2;
	}
	const auto delay = std::chrono::milliseconds(argc > 2 ? std::atoi(argv[2]) : 300);
	const std::string templates = argc > 3 ? argv[3] : "tests/integration/templates";

	App app;
	app.set_templates(templates);

	app.view<Dashboard>("/dashboard",
	                    [delay](Request&) -> View<Dashboard>
	                    {
							co_return ViewResult<Dashboard>{
								.templates = ViewTemplates("dashboard"),
								.model =
									Dashboard{
										.title = "Overview",
										.visitors_now = 17,
										// Starts running here, not when the page asks for
										// it, so it overlaps the render rather than
										// following it.
										.slow_report = Deferred<std::string>(slow_value(delay)),
									},
							};
						});

	app.run(static_cast<uint16_t>(std::atoi(argv[1])));
	return 0;
}
