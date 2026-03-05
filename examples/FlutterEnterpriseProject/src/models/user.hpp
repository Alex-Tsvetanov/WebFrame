#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <nlohmann/json.hpp>

namespace project::models
{

	struct User
	{
		int64_t id = 0;
		std::string username;
		std::string email;
		std::string password_hash;  // Never expose in JSON
		int64_t created_at = 0;

		// Serialization (excludes password_hash)
		[[nodiscard]] nlohmann::json to_json() const;
		[[nodiscard]] static User from_json(const nlohmann::json& j);

		// Validation
		[[nodiscard]] static std::optional<std::string> validate(const nlohmann::json& j);
	};

	// Request/Response DTOs
	struct LoginRequest
	{
		std::string username;
		std::string password;

		[[nodiscard]] static LoginRequest from_json(const nlohmann::json& j);
		[[nodiscard]] static std::optional<std::string> validate(const nlohmann::json& j);
	};

	struct RegisterRequest
	{
		std::string username;
		std::string email;
		std::string password;

		[[nodiscard]] static RegisterRequest from_json(const nlohmann::json& j);
		[[nodiscard]] static std::optional<std::string> validate(const nlohmann::json& j);
	};

}  // namespace project::models
