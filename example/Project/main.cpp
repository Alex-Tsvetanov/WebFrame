#include <core/core.hpp>
#include <iostream>
#include <thread>

#include "./routes/home.hpp"
#include "./routes/home_with_render.hpp"

int main([[maybe_unused]] int args, [[maybe_unused]] char** argv) {
	webframe::core::application app;

	// setup
#ifdef USE_INJA
	app.set_templates("./example/Project/static/templates");
#endif
	app.extend_with(home);
	app.extend_with(home_with_render(app), "/render");

	// start
	const char* port = "8889";
	const unsigned int cores =
	    ((std::thread::hardware_concurrency() - 1 > 0) ? (std::thread::hardware_concurrency() - 1) : 1);
	app.run(port, cores).wait_end(port);
	return 0;
}