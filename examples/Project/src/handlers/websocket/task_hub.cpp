#include "task_hub.hpp"
#include <iostream>

namespace project::handlers::websocket {

int TaskHub::add_connection(coroute::WebSocketConnection* conn) {
    std::lock_guard lock(mutex_);
    int id = next_id_++;
    connections_[id] = conn;
    std::cout << "[WebSocket] Client connected (id=" << id << ", total=" << connections_.size() << ")\n";
    return id;
}

void TaskHub::remove_connection(int id) {
    std::lock_guard lock(mutex_);
    connections_.erase(id);
    std::cout << "[WebSocket] Client disconnected (id=" << id << ", total=" << connections_.size() << ")\n";
}

void TaskHub::broadcast(const std::string& message) {
    // Queue message for all connections - will be sent by their handler loops
    std::lock_guard lock(mutex_);
    for (auto& [id, conn] : connections_) {
        pending_messages_[id].push(message);
    }
}

std::optional<std::string> TaskHub::pop_message(int id) {
    std::lock_guard lock(mutex_);
    auto it = pending_messages_.find(id);
    if (it == pending_messages_.end() || it->second.empty()) {
        return std::nullopt;
    }
    auto msg = std::move(it->second.front());
    it->second.pop();
    return msg;
}

size_t TaskHub::connection_count() const {
    std::lock_guard lock(mutex_);
    return connections_.size();
}

void register_routes(coroute::App& app, TaskHub& hub) {
    app.ws("/ws", [&hub](std::unique_ptr<coroute::WebSocketConnection> conn) -> coroute::Task<void> {
        auto* conn_ptr = conn.get();
        int conn_id = hub.add_connection(conn_ptr);
        
        try {
            // Send welcome message
            nlohmann::json welcome;
            welcome["type"] = "connected";
            welcome["message"] = "Connected to Task Dashboard";
            auto send_result = co_await conn->send_text(welcome.dump());
            if (!send_result) {
                hub.remove_connection(conn_id);
                co_return;
            }
            
            // Handle incoming messages
            while (conn->is_open()) {
                // Send any pending broadcast messages first
                while (auto pending = hub.pop_message(conn_id)) {
                    auto result = co_await conn->send_text(*pending);
                    if (!result) {
                        hub.remove_connection(conn_id);
                        co_return;
                    }
                }
                
                auto msg_result = co_await conn->receive();
                
                if (!msg_result) {
                    break;
                }
                
                auto& msg = *msg_result;
                
                if (msg.opcode == coroute::WebSocketOpcode::Close) {
                    break;
                }
                
                if (msg.opcode == coroute::WebSocketOpcode::Text) {
                    try {
                        std::string text(msg.text());
                        auto data = nlohmann::json::parse(text);
                        
                        // Handle ping - respond with pong
                        if (data.value("type", "") == "ping") {
                            nlohmann::json pong;
                            pong["type"] = "pong";
                            co_await conn->send_text(pong.dump());
                        }
                    } catch (...) {
                        // Ignore invalid JSON
                    }
                }
            }
        } catch (...) {
            // Connection closed or error
        }
        
        // IMPORTANT: Remove from hub BEFORE conn is destroyed
        hub.remove_connection(conn_id);
        co_return;
    });
}

} // namespace project::handlers::websocket
