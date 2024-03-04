#pragma once

#include <ios>

namespace webframe::exceptions {
	class unable_to_bind_socket : public std::ios_base::failure {
	public:
		explicit unable_to_bind_socket() : failure("Socket was unable to bind.") {}
	};
}  // namespace webframe::exceptions