#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

#include "../../c/host.h"
#include "../../utils/generator.hpp"

namespace webframe::core {
	struct thread_pool {
	private:
		size_t size;
		std::shared_ptr<utils::generator<SOCKET>> client_getter;
		std::shared_ptr<std::mutex> getter_mutex;
		std::shared_ptr<std::atomic<bool>> stop_flag;
		std::shared_ptr<std::optional<size_t>> requests;
		std::vector<std::thread> threads;
		std::mutex join_mutex;
		bool joined = false;

	public:
		template <typename T>
		explicit thread_pool(size_t _size, std::shared_ptr<utils::generator<SOCKET>> getter,
		                     std::shared_ptr<std::mutex> getter_mutex,
		                     std::shared_ptr<std::atomic<bool>> stop_flag, T worker,
		                     std::shared_ptr<std::optional<size_t>> requests) {
			this->size = _size;
			this->client_getter = getter;
			this->getter_mutex = getter_mutex;
			this->stop_flag = stop_flag;
			this->requests = requests;
			this->threads.reserve(this->size);
			for (size_t i = 0; i < this->size; i++) {
				this->threads.push_back(std::thread(worker, this->client_getter, this->getter_mutex,
				                              this->stop_flag, this->requests));
			}
		}

		void start() {}

		void stop() {
			if (this->stop_flag) this->stop_flag->store(true);
		}

		void join() {
			std::lock_guard<std::mutex> lock(this->join_mutex);
			if (this->joined) return;
			for (auto& thread : this->threads) {
				if (thread.joinable()) thread.join();
			}
			this->joined = true;
		}

		~thread_pool() {
			this->stop();
			this->join();
		}
	};

}  // namespace webframe::core