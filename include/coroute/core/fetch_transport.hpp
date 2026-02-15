#ifndef COROUTE_CORE_FETCH_TRANSPORT_HPP
#define COROUTE_CORE_FETCH_TRANSPORT_HPP

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

	// Abstract interface for fetching resources (HTTP Client)
	class FetchTransport {
	public:
		virtual ~FetchTransport() = default;
		virtual Task<Response> dispatch(const Request& req) = 0;
	};

}  // namespace coroute

#endif  // COROUTE_CORE_FETCH_TRANSPORT_HPP
