#pragma once
#include <string>
#include <iostream>

class response_of {
	std::string response;
	double time;
	std::string current;
public:
	response_of(const std::string& url, const std::string& uuid) {
		current = uuid;
		const std::string command = "curl -sS -w \"%{time_total}\" " + url + " -o ./bin/log/curl" + current + ".txt > ./bin/log/time" + current + ".txt 2> ./bin/log/curl_logs" + current + ".txt";
		
		const int code [[maybe_unused]] = system(command.c_str());
		{
			std::ifstream fin("./bin/log/curl" + current + ".txt");
			std::stringstream buffer;
			buffer << fin.rdbuf();
			response = buffer.str();
		}
		{
			std::ifstream fin("./bin/log/time" + current + ".txt");
			fin >> time;
		}
	}

	response_of& must_be(const std::string& text) {
		REQUIRE_THAT(response, Equals(text));
		return *this;
	}

	response_of& might_take_less(double max_time) {
		CHECK(time < max_time);
		return *this;
	}
};