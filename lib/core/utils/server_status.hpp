/**
 *  @file   server_status.hpp
 *  @brief  The class will manage the status of the server ran on each port
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#pragma once
#include "../http/predef.hpp"
#include <shared_mutex>
#include <condition_variable>

namespace webframe::utils {
class server_status {
public:
	server_status() { }
	
	class port {
	public:
		enum Status : short {
			NOT_STARTED = 0,
			STARTED,
			RUNNING,
			SIGNALED,
			STOPPED
		};
		
		Status status;
		std::shared_ptr<std::mutex> m;
		std::condition_variable cv;
		std::shared_ptr<std::optional<size_t>> number_of_requests;
		
		port() {
			status = Status::NOT_STARTED;
			m = std::make_shared<std::mutex>();
			number_of_requests = nullptr;
		}

		void change_status(Status s) {
			std::unique_lock lk(*m);
			this->status = s;
			lk.unlock();
			this->cv.notify_all();
		}

		bool is_started() {
			return this->status == port::Status::STARTED;
		}
		bool is_stopped() {
			return this->status == port::Status::STOPPED;
		}
		
		inline void wait_to_start() {
			std::unique_lock lk(*m);
			cv.wait(lk, [this](){ return this->is_started(); });
		}

		inline void wait_to_stop() {
			std::unique_lock lk(*m);
			cv.wait(lk, [this](){ return this->is_stopped(); });
		}
	};

private:	
	port ports[65536];
public:
	void initiate(const char *PORT, std::optional<size_t> requests = std::nullopt) {
		if (ports[number(PORT)].status == port::Status::RUNNING || ports[number(PORT)].status == port::Status::STARTED || ports[number(PORT)].status == port::Status::SIGNALED)
			throw "The port is already/still in-use";
		ports[number(PORT)].number_of_requests = std::make_shared<std::optional<size_t>>(requests);
		change_status(PORT, port::Status::STARTED);
	}

	inline void change_status(const char *PORT, port::Status s) {
		ports[number(PORT)].change_status(s);
	}

	inline port::Status get_status(const char *PORT) {
		return ports[number(PORT)].status;
	}

	inline void wait_to_start(const char *PORT) {
		ports[number(PORT)].wait_to_start();
	}

	inline void wait_to_stop(const char *PORT) {
		ports[number(PORT)].wait_to_stop();
	}

	inline void signal_to_stop(const char *PORT) {
		change_status(PORT, port::Status::SIGNALED);
	}

	inline std::shared_ptr<std::optional<size_t>> requests_left(const char* PORT) {
		return ports[number(PORT)].number_of_requests;
	}
private:
	static constexpr size_t number(const char* PORT) {
		return _compile_time::string_to_uint(PORT);
	}
};
} // namespace webframe::utils