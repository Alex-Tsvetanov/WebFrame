#include "coroute/net/udp_server.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace coroute::net {

// ============================================================================
// UdpConnection
// ============================================================================

UdpConnection::UdpConnection(UdpServer* server, UdpEndpoint peer)
    : server_(server), peer_endpoint_(std::move(peer)) {}

UdpConnection::~UdpConnection() {
    close();
}

void UdpConnection::on_datagram(std::vector<uint8_t> data) {
    if (closed_) return;
    pending_datagrams_.push(std::move(data));
    if (read_waiter_) {
        auto h = read_waiter_;
        read_waiter_ = nullptr;
        h.resume();
    }
}

class UdpReadAwaiter {
    UdpConnection* conn_;
public:
    UdpReadAwaiter(UdpConnection* c) : conn_(c) {}
    bool await_ready() const { return !conn_->pending_datagrams_.empty() || conn_->closed_; }
    void await_suspend(std::coroutine_handle<> h) { conn_->read_waiter_ = h; }
    std::vector<uint8_t> await_resume() {
        if (conn_->closed_ || conn_->pending_datagrams_.empty()) {
            return {};
        }
        auto data = std::move(conn_->pending_datagrams_.front());
        conn_->pending_datagrams_.pop();
        return data;
    }
};

Task<expected<size_t, Error>> UdpConnection::async_read(void* buffer, size_t length) {
    if (closed_) {
        co_return unexpected(Error::io(IoError::ConnectionAborted, "Connection closed"));
    }

    // Wait for at least one datagram
    auto datagram = co_await UdpReadAwaiter(this);
    if (datagram.empty()) {
        co_return unexpected(Error::io(IoError::ConnectionAborted, "Connection closed"));
    }

    size_t copy_len = std::min(length, datagram.size());
    std::memcpy(buffer, datagram.data(), copy_len);

    // Note: UDP is record-oriented. If we read less than datagram size, the rest is discarded.
    // This is true for recv() on SOCK_DGRAM as well.
    co_return copy_len;
}

Task<expected<size_t, Error>> UdpConnection::async_read_until(void* buffer, size_t len, char delimiter) {
    (void)delimiter;
    // Basic implementation for UDP: just read a datagram
    co_return co_await async_read(buffer, len);
}

Task<expected<size_t, Error>> UdpConnection::async_write(const void* buffer, size_t length) {
    if (closed_) {
        co_return unexpected(Error::io(IoError::ConnectionAborted, "Connection closed"));
    }
    co_return co_await server_->send_to(buffer, length, peer_endpoint_);
}

Task<expected<size_t, Error>> UdpConnection::async_write_all(const void* buffer, size_t length) {
    co_return co_await async_write(buffer, length);
}

Task<expected<size_t, Error>> UdpConnection::async_transmit_file(FileHandle file, size_t offset, size_t length) {
    (void)file;
    (void)offset;
    (void)length;
    co_return unexpected(Error::io(IoError::InvalidArgument, "async_transmit_file not supported on UDP"));
}

void UdpConnection::close() {
    if (closed_) return;
    closed_ = true;
    if (read_waiter_) {
        auto h = read_waiter_;
        read_waiter_ = nullptr;
        h.resume();
    }
    if (server_) {
        server_->remove_connection(peer_endpoint_);
        server_ = nullptr;
    }
}

bool UdpConnection::is_open() const noexcept {
    return !closed_;
}

void UdpConnection::set_timeout(std::chrono::milliseconds) {
}

std::string UdpConnection::remote_address() const {
    return peer_endpoint_.address;
}

uint16_t UdpConnection::remote_port() const noexcept {
    return peer_endpoint_.port;
}

void UdpConnection::set_cancellation_token(CancellationToken) {
}

// ============================================================================
// UdpServer
// ============================================================================

UdpServer::UdpServer(IoContext& ctx) : ctx_(ctx) {}

UdpServer::~UdpServer() {
    stopped_ = true;
    if (accept_waiter_) {
        accept_waiter_.resume();
    }
}

expected<void, Error> UdpServer::listen(uint16_t port) {
    auto sock = UdpSocket::create(ctx_);
    auto bind_res = sock->bind(port);
    if (!bind_res) {
        return unexpected(bind_res.error());
    }
    socket_ = std::move(sock);
    return {};
}

class UdpAcceptAwaiter {
    UdpServer* server_;
public:
    UdpAcceptAwaiter(UdpServer* s) : server_(s) {}
    bool await_ready() const { return !server_->new_connections_.empty() || server_->stopped_; }
    void await_suspend(std::coroutine_handle<> h) { server_->accept_waiter_ = h; }
    expected<std::unique_ptr<Connection>, Error> await_resume() {
        if (server_->stopped_ || server_->new_connections_.empty()) {
            return unexpected(Error::io(IoError::ConnectionAborted, "Server closed"));
        }
        auto conn = std::move(server_->new_connections_.front());
        server_->new_connections_.pop();
        return conn;
    }
};

Task<expected<std::unique_ptr<Connection>, Error>> UdpServer::async_accept() {
    co_return co_await UdpAcceptAwaiter(this);
}

Task<expected<size_t, Error>> UdpServer::send_to(const void* buffer, size_t length, const UdpEndpoint& peer) {
    if (!socket_) co_return unexpected(Error::io(IoError::InvalidArgument, "Socket not listening"));
    co_return co_await socket_->async_send_to(buffer, length, peer);
}

IoContext& UdpServer::context() {
    return ctx_;
}

void UdpServer::remove_connection(const UdpEndpoint& peer) {
    std::string key = peer.address + ":" + std::to_string(peer.port);
    connections_.erase(key);
}

Task<void> UdpServer::run() {
    if (!socket_) {
        co_return;
    }

    std::vector<uint8_t> buffer(65536);

    while (!stopped_) {
        auto res = co_await socket_->async_recv_from(buffer.data(), buffer.size());
        if (!res) {
            // Error, could be closed or temporary error
            std::cerr << "UDP Server recv error: " << res.error().to_string() << "\n";
            continue; // Could be transient, keep trying
        }

        auto [bytes_read, peer_ep] = *res;
        std::string key = peer_ep.address + ":" + std::to_string(peer_ep.port);

        auto it = connections_.find(key);
        UdpConnection* conn = nullptr;

        if (it == connections_.end()) {
            // New connection!
            auto new_conn = std::make_unique<UdpConnection>(this, peer_ep);
            conn = new_conn.get();
            connections_[key] = conn;

            new_connections_.push(std::move(new_conn));
            if (accept_waiter_) {
                auto h = accept_waiter_;
                accept_waiter_ = nullptr;
                h.resume();
            }
        } else {
            conn = it->second;
        }

        // Pass the datagram to the connection
        std::vector<uint8_t> data(buffer.data(), buffer.data() + bytes_read);
        conn->on_datagram(std::move(data));
    }
}

} // namespace coroute::net
