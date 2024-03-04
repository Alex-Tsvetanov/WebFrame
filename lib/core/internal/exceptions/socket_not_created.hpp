#pragma once

#include <ios>

namespace webframe::exceptions {
	class socket_not_created : public std::ios_base::failure {
	public:
		explicit socket_not_created() : failure("Socket failed to create.") {}
	};
}  // namespace webframe::exceptions