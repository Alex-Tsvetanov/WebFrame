#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace webframe::core {
	/**
	 *  @brief   Type of path prameters
	 *  @details This is the type of all variables passed as part of the unique URL
	 *  @note    Ex. In 'url/variable' the value of the variable will be saved in
	 *the variable.
	 ***********************************************/
	struct path_vars {
		struct var {
			std::string value;
			std::string type;

			var();

			explicit var(const std::string& _value);

			var(const std::string& _value, const std::string& _type);
			var(const std::pair<std::string, std::string>& details);

			const std::string& get() const;

			explicit operator int() const;
			explicit operator long long() const;
			explicit operator const char*() const;
			explicit operator char() const;
			explicit operator std::string() const;
			explicit operator const std::string&() const;
			template <typename T>
			explicit operator T&() const;
			template <typename T>
			explicit operator T() const;
		};
		path_vars();
		path_vars(std::initializer_list<var> l);

	private:
		std::vector<var> vars;

	public:
		const var operator[](long long unsigned int ind) const;
		path_vars& operator+=(const var& v);
		size_t size() const;
	};
}  // namespace webframe::core

#include "path_variables.cpp"