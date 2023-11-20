#pragma once
#include "../core.hpp"
#include <string>
#include <vector>

namespace webframe::core 
{
	/**
		*  @brief   Type of path prameters
		*  @details This is the type of all variables passed as part of the unique URL
		*  @note    Ex. In 'url/variable' the value of the variable will be saved in the variable.
		***********************************************/
	struct path_vars
	{
		struct var
		{
			std::string value;
			std::string type;

			var() : value(""), type("string")
			{ }

			explicit var(const std::string& _value) : value(_value), type("string")
			{ }

			var(const std::string& _value, const std::string& _type) : value(_value), type(_type)
			{ }

			const std::string& get() const
			{
				return value;
			}

			explicit operator int() const
			{
				if (value.size() == 0)
					throw std::invalid_argument("path_vars::var::value is empty.");
				int ans = 0;
				if (value[0] == '-')
					ans = -(value[1] - '0');
				for (size_t i = (value[0] == '-'); i < value.size(); i++)
					if (value[i] >= '0' and value[i] <= '9')
						ans = ans * 10 + value[i] - '0';
					else
						throw std::invalid_argument("path_vars::var::value is not matching path_vars::var::type (not integer)");
				return ans;
			}
			explicit operator long long() const
			{
				if (value.size() == 0)
					throw std::invalid_argument("path_vars::var::value is empty.");
				int ans = 0;
				if (value[0] == '-')
					ans = -(value[1] - '0');
				for (size_t i = (value[0] == '-'); i < value.size(); i++)
					if (value[i] >= '0' and value[i] <= '9')
						ans = ans * 10 + value[i] - '0';
					else
						throw std::invalid_argument("path_vars::var::value is not matching path_vars::var::type (not integer)");
				return ans;
			}
			explicit operator const char* () const
			{
				if (value.size() == 0)
					throw std::invalid_argument("path_vars::var::value is empty.");
				return value.c_str();
			}
			explicit operator char() const
			{
				if (value.size() == 0)
					throw std::invalid_argument("path_vars::var::value is empty.");
				if (value.size() != 1)
					throw std::invalid_argument("path_vars::var::value is too long.");
				return value[0];
			}
			explicit operator std::string() const
			{
				if (value.size() == 0)
					throw std::invalid_argument("path_vars::var::value is empty.");
				return value;
			}
			explicit operator const std::string& () const
			{
				if (value.size() == 0)
					throw std::invalid_argument("path_vars::var::value is empty.");
				return value;
			}
			template<typename T>
			explicit operator T& () const {
				T* ans = new T(value);
				return *ans;
			}
			template<typename T>
			explicit operator T() const {
				T* ans = new T(value);
				return *ans;
			}
		};
		path_vars() {}
	private:
		std::vector < var > vars;
	public:
		const var operator[] (long long unsigned int ind) const
		{
			return vars[ind];
		}
		path_vars& operator += (const var& v)
		{
			vars.push_back(v);
			return *this;
		}
		size_t size() const
		{
			return vars.size();
		}
	};
}