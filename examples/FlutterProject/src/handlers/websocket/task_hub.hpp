#pragma once

#include "coroute/core/app.hpp"
#include "coroute/net/websocket.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <queue>
#include <optional>

namespace flutter_project::handlers::websocket
{

	// Manages WebSocket connections for real-time task updates
	class TaskHub
	{
	public:
		TaskHub() = default;

		// Connection management (stores raw pointers, lifetime managed by handler)
		int add_connection(coroute::WebSocketConnection* conn);
		void remove_connection(int id);

		// Broadcasting - queues messages for all WebSocket connections and fires
		// app.fire_broadcast() so the FFI bridge can push events to Flutter.
		void broadcast(const std::string& message);

		// Get next pending message for a connection
		std::optional<std::string> pop_message(int id);

		// Stats
		size_t connection_count() const;

	private:
		std::unordered_map<int, coroute::WebSocketConnection*> connections_;
		std::unordered_map<int, std::queue<std::string>> pending_messages_;
		mutable std::mutex mutex_;
		std::atomic<int> next_id_{1};
	};

	// Register WebSocket routes
	void register_routes(coroute::App& app, TaskHub& hub);

}  // namespace flutter_project::handlers::websocket
