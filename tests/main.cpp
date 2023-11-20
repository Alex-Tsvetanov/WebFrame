#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using Catch::Matchers::StartsWith;
using Catch::Matchers::EndsWith;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::Equals;

#include <core/core.hpp>

#include "IT/response_of.hpp"

std::ostream* null;

SCENARIO("application responser can respond with the correct header") {
	GIVEN("Standard application with /") {
		
		webframe::core::_application& app = webframe::core::create_app();
		app
			.route("/", []() {
				return "";
			});
		
		THEN("responser should respond with status code 200 by default") {
			auto r = (*app.get_routes().begin()).second.call("1.1", "", webframe::core::path_vars()).to_string();
			REQUIRE_THAT(r,	StartsWith("HTTP/1.1 200 OK"));
		}

		delete &app;
	}
	GIVEN("Internl server error within a request") {
		webframe::core::_application& app = webframe::core::create_app();
		app
			.route("/", []() {
				return webframe::core::response(webframe::core::status_line("500"), { {"Content-Type", "text/html; charset=utf-8"} }, "");
			});

		THEN("responser should respond with status code 200 by default") {
			auto r = (*app.get_routes().begin()).second.call("1.1", "", webframe::core::path_vars()).to_string();
			REQUIRE_THAT(r, StartsWith("HTTP/1.1 500 Internal Server Error"));
		}

		delete &app;
	}
}

SCENARIO("application responser can respond with the correct body") {
	GIVEN("Standard application with / and body \"sample\"") {
		webframe::core::_application& app = webframe::core::create_app();
		app
			.route("/", []() {
				return "sample";
			});

		THEN("responser should respond with correct HTML") {
			auto r = (*app.get_routes().begin()).second.call("1.1", "", webframe::core::path_vars()).to_string();
			REQUIRE_THAT(r, ContainsSubstring("sample"));
		}

		delete &app;
	}
	GIVEN("Standard application with /{user} and body {{user}}") {
		webframe::core::_application& app = webframe::core::create_app();
		app
			.route("/{text}", [](std::string username) {
				return username;
			});

		THEN("responser should respond with the given path variable value") {
			auto params = webframe::core::path_vars();
			params += {"username", "string"};

			auto r = (*app.get_routes().begin()).second.call("1.1", "", params).to_string();
			REQUIRE_THAT(r, ContainsSubstring("username"));
		}

		delete &app;
	}
}

SCENARIO("Application responds correct") {
	GIVEN("Standart application with \"<h1>Hello, World!</h1>\" on / and /{number}") {
		webframe::core::application app;
		app
			.set_logger(null)
			.set_performancer(null)
			.route("/", []() {
				return webframe::core::response(webframe::core::status_line("1.1", "200"), { {"Content-Type", "text/html; charset=utf-8"} }, "<h1>Hello, World!</h1>");
			})
			.route("/{number}", [](int steps) {
				volatile unsigned long long int test = 0;
				for (int i = 0; i < (1 << steps); i++)
				{
					test++;
				}
				return "Hello, World!";
			})
			.run("8890", 1)
			.wait_start("8890");

		const std::string numbers[] = { "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31" };
		std::vector<response_of> benchmarking;
		for (int i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {
			benchmarking.push_back(response_of("http://localhost:8890/" + numbers[i], numbers[i]));
		}

		THEN("Responds with \"<h1>Hello, World!</h1>\" for /") {
			response_of("http://localhost:8890/", "basic").must_be("<h1>Hello, World!</h1>");
			/*for (int i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {
				INFO("Request with " << (1ll << i) << " should take less than " << (15.0 * 1e-6 * (1ll << i)) << "s and should respond with \"Hello, World!\"");
				benchmarking[i].must_be("Hello, World!").might_take_less(15.0 * 1e-6 * (1ll << i));
			}*/
		}

		app.request_stop("8890");
		app.wait_end("8890");
	}
}

TEMPLATE_TEST_CASE_SIG("Standart application with \"Hello, World!\" on /{number} for number", "[IT]",
	((int V), V), (0), (1), (2), (3), (4), (5), (6), (7), (8), (9), (10), (11), (12), (13), (14), (15), (16), (17), (18), (19), (20), (21), (22), (23), (24), (25), (26), (27), (28), (29), (30), (31)) {
	int i = V;
	std::stringstream ss;
	ss << i;
	std::string number = ss.str();
	
	response_of("http://localhost:8891/" + number, number).must_be("Hello, World!").might_take_less(15.0 * 1e-6 * (1ll << i));
}

int main (int argc, char** argv)
{
	static_assert(webframe::core::application::init(), "constexpr initiation failed");
	
	std::filebuf null_f;
	null_f.open("./bin/log/null.txt", std::ios_base::out);
	std::ostream nul(&null_f);
	null = &nul;

	webframe::core::application app;
	app
		.set_logger(&nul)
		.set_performancer(&nul)
		.route("/{number}", [](int steps) {
			volatile unsigned long long int test = 0;
			for (int i = 0; i < (1 << steps); i++)
			{
				test++;
			}
			return "Hello, World!";
		})
		.run("8891", 1)
		.wait_start("8891");

	int result = Catch::Session().run(argc, argv);

	return 0;
}