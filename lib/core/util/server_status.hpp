/** 
 *  @file   server_status.hpp 
 *  @brief  The class will manage the status of the server ran on each port
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#pragma once
#include "../http_consts/constexpr.hpp"
#include <shared_mutex>

namespace webframe 
{
	class server_status 
	{
	public:
        server_status() 
		{
			for (int i = 0 ; i < 65536 ; i++) 
			{
				start[i] = dead[i] = nullptr;
			}
        }
    private:
#ifdef USE_PTHREAD
		using mutex = std::mutex;
#else
		using mutex = std::shared_mutex;
#endif
		mutex* start [65536];
		mutex* dead  [65536];
		
    public:
		void initiate(const char* PORT) 
		{
			if (this->get_start_ptr(PORT) != nullptr)
				throw std::ios_base::failure("start mutex is not cleaned up.");
			if (this->get_end_ptr(PORT) != nullptr)
				throw std::ios_base::failure("end mutex is not cleaned up.");
			this->get_start_ptr(PORT) = new mutex();
			this->get_end_ptr(PORT) = new mutex();
			this->lock_working(PORT);
			this->lock_dead(PORT);
		}

        void alert_start(const char* PORT) 
		{
            this->unlock_working(PORT);
        }

        void alert_end(const char* PORT) 
		{
            this->unlock_dead(PORT);
        }

        bool is_over(const char* PORT) 
		{
            bool locked = this->get_end(PORT).try_lock();
			if(!locked) return false;
			this->get_end(PORT).unlock();
			return true;
        }

        mutex& get_start(const char* PORT) 
		{
            return *this->start[_compile_time::string_to_uint(PORT)];
        }

        mutex& get_end(const char* PORT) 
		{
            return *this->dead[_compile_time::string_to_uint(PORT)];
        }

		void reset (const char* PORT) 
		{
			delete this->get_start_ptr(PORT);
			delete this->get_end_ptr(PORT);
			this->get_start_ptr(PORT) = nullptr;
			this->get_end_ptr(PORT) = nullptr;
		}

    private:
        mutex*& get_start_ptr(const char* PORT) 
		{
            return this->start[_compile_time::string_to_uint(PORT)];
        }

        mutex*& get_end_ptr(const char* PORT) 
		{
            return this->dead[_compile_time::string_to_uint(PORT)];
        }

		void lock_working(const char* PORT) 
		{
			this->start[_compile_time::string_to_uint(PORT)]->lock();
		}

		void unlock_working(const char* PORT) 
		{
			this->start[_compile_time::string_to_uint(PORT)]->unlock();
		}

		void lock_dead(const char* PORT) 
		{
			this->dead[_compile_time::string_to_uint(PORT)]->lock();
		}

		void unlock_dead(const char* PORT) 
		{
			this->dead[_compile_time::string_to_uint(PORT)]->unlock();
		}
	};
}