/**
 * Coroute v2 - WebSocket Example
 * 
 * Demonstrates WebSocket support with echo and broadcast functionality.
 */

#include <coroute/coroute.hpp>
#include <iostream>
#include <set>
#include <mutex>

using namespace coroute;

// Simple chat room - tracks connected clients
class ChatRoom {
    std::mutex mutex_;
    std::set<WebSocketConnection*> clients_;
    
public:
    void join(WebSocketConnection* client) {
        std::lock_guard lock(mutex_);
        clients_.insert(client);
        std::cout << "Client joined. Total: " << clients_.size() << std::endl;
    }
    
    void leave(WebSocketConnection* client) {
        std::lock_guard lock(mutex_);
        clients_.erase(client);
        std::cout << "Client left. Total: " << clients_.size() << std::endl;
    }
    
    // Broadcast message to all clients except sender
    Task<void> broadcast(WebSocketConnection* sender, std::string_view message) {
        std::vector<WebSocketConnection*> targets;
        {
            std::lock_guard lock(mutex_);
            for (auto* client : clients_) {
                if (client != sender && client->is_open()) {
                    targets.push_back(client);
                }
            }
        }
        
        for (auto* client : targets) {
            co_await client->send_text(message);
        }
    }
};

// Global chat room
ChatRoom chat_room;

int main() {
    App app;
    
    app.threads(4);
    
    // Serve a simple HTML page for testing
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::html(R"html(
<!DOCTYPE html>
<html>
<head>
    <title>WebSocket Test</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; }
        #messages { height: 300px; border: 1px solid #ccc; overflow-y: scroll; padding: 10px; margin-bottom: 10px; }
        #input { width: 80%; padding: 10px; }
        button { padding: 10px 20px; }
        .system { color: #888; font-style: italic; }
        .sent { color: blue; }
        .received { color: green; }
    </style>
</head>
<body>
    <h1>WebSocket Chat</h1>
    <div id="messages"></div>
    <input type="text" id="input" placeholder="Type a message..." onkeypress="if(event.key==='Enter')send()">
    <button onclick="send()">Send</button>
    <button onclick="ping()">Ping</button>
    
    <h2>Echo Test</h2>
    <p>Connect to <code>/echo</code> for a simple echo server.</p>
    <p>Connect to <code>/chat</code> for the chat room.</p>
    
    <script>
        var messages = document.getElementById("messages");
        var input = document.getElementById("input");
        var ws;
        
        function log(msg, className) {
            var div = document.createElement("div");
            div.textContent = msg;
            if (className) div.className = className;
            messages.appendChild(div);
            messages.scrollTop = messages.scrollHeight;
        }
        
        function connect() {
            var protocol = location.protocol === "https:" ? "wss:" : "ws:";
            ws = new WebSocket(protocol + "//" + location.host + "/chat");
            
            ws.onopen = function() { log("Connected to chat room", "system"); };
            ws.onclose = function() { log("Disconnected", "system"); };
            ws.onerror = function(e) { log("Error", "system"); };
            ws.onmessage = function(e) { log("Received: " + e.data, "received"); };
        }
        
        function send() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                var msg = input.value;
                ws.send(msg);
                log("Sent: " + msg, "sent");
                input.value = "";
            }
        }
        
        function ping() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send("PING");
                log("Sent ping", "system");
            }
        }
        
        connect();
    </script>
</body>
</html>
)html");
    });
    
    // Echo WebSocket endpoint - echoes back any message received
    app.ws("/echo", [](std::unique_ptr<WebSocketConnection> ws) -> Task<void> {
        std::cout << "Echo client connected from " << ws->remote_address() << std::endl;
        
        while (ws->is_open()) {
            auto msg = co_await ws->receive();
            if (!msg) {
                std::cout << "Echo client disconnected: " << msg.error().to_string() << std::endl;
                break;
            }
            
            if (msg->is_text()) {
                std::cout << "Echo: " << msg->text() << std::endl;
                co_await ws->send_text(msg->text());
            } else if (msg->is_binary()) {
                co_await ws->send_binary(msg->data);
            } else if (msg->is_close()) {
                std::cout << "Echo client sent close" << std::endl;
                break;
            }
        }
    });
    
    // Chat WebSocket endpoint - broadcasts messages to all connected clients
    app.ws("/chat", [](std::unique_ptr<WebSocketConnection> ws) -> Task<void> {
        std::cout << "Chat client connected from " << ws->remote_address() << std::endl;
        
        auto* ws_ptr = ws.get();
        chat_room.join(ws_ptr);
        
        // Send welcome message
        co_await ws->send_text("Welcome to the chat room!");
        
        while (ws->is_open()) {
            auto msg = co_await ws->receive();
            if (!msg) {
                break;
            }
            
            if (msg->is_text()) {
                std::string text(msg->text());
                std::cout << "Chat message: " << text << std::endl;
                
                // Broadcast to all other clients
                co_await chat_room.broadcast(ws_ptr, text);
                
                // Echo back to sender with confirmation
                co_await ws->send_text("You said: " + text);
            } else if (msg->is_close()) {
                break;
            }
        }
        
        chat_room.leave(ws_ptr);
        std::cout << "Chat client disconnected" << std::endl;
    });
    
    std::cout << "WebSocket Server starting on port 8080..." << std::endl;
    std::cout << "Open http://localhost:8080/ in your browser" << std::endl;
    std::cout << std::endl;
    std::cout << "WebSocket endpoints:" << std::endl;
    std::cout << "  ws://localhost:8080/echo - Echo server" << std::endl;
    std::cout << "  ws://localhost:8080/chat - Chat room" << std::endl;
    
    app.run(8080);
    
    return 0;
}
