#pragma once

#include <ios>

namespace webframe::exceptions {
	class unable_to_retrieve_address : public std::ios_base::failure {
	public:
		explicit unable_to_retrieve_address() : failure("Address failed to be retrieved.") {}
	};
}  // namespace webframe::exceptions