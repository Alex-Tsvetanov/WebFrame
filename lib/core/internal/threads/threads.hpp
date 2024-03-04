#pragma once

#include "../../c/host.h"
#include "../../utils/generator.hpp"
#include <memory>
#include <optional>

namespace webframe::core {
		struct thread_pool {
		private:
			size_t size;
			std::shared_ptr<utils::generator<SOCKET>> client_getter;
			std::shared_ptr<std::optional<size_t>> requests;
			std::vector<std::thread> threads;
		public:
			template<typename T>
			explicit thread_pool(size_t _size, std::shared_ptr<utils::generator<SOCKET>> getter, T worker, std::shared_ptr<std::optional<size_t>> requests) {
				this->size = _size;
				this->client_getter = getter;
				this->requests = requests;
				this->threads.reserve(this->size);
				for (size_t i = 0 ; i < this->size ; i ++) {
					this->threads.push_back(std::thread(worker, this->client_getter, this->requests));
				}
			}
			
			void start() {
				for (size_t i = 0 ; i < this->size ; i ++) {
					this->threads[i].detach();
				}
			}
		};

}