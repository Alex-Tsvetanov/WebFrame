#include "task_hub.hpp"
#include <iostream>

namespace flutter_project::handlers::websocket
{

	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	static TaskHub* g_task_hub = nullptr;

	int TaskHub::add_connection(coroute::WebSocketConnection* conn)
	{
		std::lock_guard lock(mutex_);
		int id = next_id_++;
		connections_[id] = conn;
		std::cout << "[WebSocket] Client connected (id=" << id << ", total=" << connections_.size() << ")\n";
		return id;
	}

	void TaskHub::remove_connection(int id)
	{
		std::lock_guard lock(mutex_);
		connections_.erase(id);
		pending_messages_.erase(id);
		std::cout << "[WebSocket] Client disconnected (id=" << id << ", total=" << connections_.size() << ")\n";
	}

	void TaskHub::broadcast(const std::string& message)
	{
		{
			std::lock_guard lock(mutex_);
			for (auto& [id, conn] : connections_)
			{
				pending_messages_[id].push(message);
			}
		}
		// Fire through App so bridge.cpp's registered callback reaches Flutter
		// without any project-specific FFI export.
		auto* app = coroute::App::instance();
		if (app)
		{
			app->fire_broadcast(message);
		}
	}

	std::optional<std::string> TaskHub::pop_message(int id)
	{
		std::lock_guard lock(mutex_);
		auto it = pending_messages_.find(id);
		if (it == pending_messages_.end() || it->second.empty())
		{
			return std::nullopt;
		}
		auto msg = std::move(it->second.front());
		it->second.pop();
		return msg;
	}

	size_t TaskHub::connection_count() const
	{
		std::lock_guard lock(mutex_);
		return connections_.size();
	}

	void register_routes(coroute::App& app, TaskHub& hub)
	{
		g_task_hub = &hub;

		app.ws("/ws",
		       [](std::unique_ptr<coroute::WebSocketConnection> conn) -> coroute::Task<void>
		       {
				   auto* conn_ptr = conn.get();
				   int conn_id = g_task_hub->add_connection(conn_ptr);

				   try
				   {
					   // Send welcome message
					   nlohmann::json welcome;
					   welcome["type"] = "connected";
					   welcome["message"] = "Connected to Task Dashboard";
					   auto send_result = co_await conn->send_text(welcome.dump());
					   if (!send_result)
					   {
						   g_task_hub->remove_connection(conn_id);
						   co_return;
					   }

					   // Handle incoming messages
					   while (conn->is_open())
					   {
						   // Send any pending broadcast messages first
						   while (auto pending = g_task_hub->pop_message(conn_id))
						   {
							   auto result = co_await conn->send_text(*pending);
							   if (!result)
							   {
								   g_task_hub->remove_connection(conn_id);
								   co_return;
							   }
						   }

						   auto msg_result = co_await conn->receive();

						   if (!msg_result)
						   {
							   break;
						   }

						   auto& msg = *msg_result;

						   if (msg.opcode == coroute::WebSocketOpcode::Close)
						   {
							   break;
						   }

						   if (msg.opcode == coroute::WebSocketOpcode::Text)
						   {
							   try
							   {
								   std::string text(msg.text());
								   auto data = nlohmann::json::parse(text);

								   if (data.value("type", "") == "ping")
								   {
									   nlohmann::json pong;
									   pong["type"] = "pong";
									   auto pong_result = co_await conn->send_text(pong.dump());
									   if (!pong_result)
									   {
										   g_task_hub->remove_connection(conn_id);
										   co_return;
									   }
								   }
							   }
							   catch (const std::exception& e)
							   {
								   std::cerr << "[WebSocket] JSON parse error: " << e.what() << '\n';
							   }
						   }
					   }
				   }
				   catch (const std::exception& e)
				   {
					   std::cerr << "[WebSocket] Connection error: " << e.what() << '\n';
				   }

				   g_task_hub->remove_connection(conn_id);
				   co_return;
			   });
	}

}  // namespace flutter_project::handlers::websocket
