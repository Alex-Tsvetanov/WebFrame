#pragma once

#include "coroute/net/io_context.hpp"
#include "coroute/coro/task.hpp"
#include <unordered_map>
#include <queue>
#include <vector>
#include <memory>
#include <string>

namespace coroute::net {

class UdpServer;

class UdpConnection : public Connection {
public:
    UdpConnection(UdpServer* server, UdpEndpoint peer_endpoint);
    ~UdpConnection() override;

    Task<expected<size_t, Error>> async_read(void* buffer, size_t length) override;
    Task<expected<size_t, Error>> async_read_until(void* buffer, size_t len, char delimiter) override;
    Task<expected<size_t, Error>> async_write(const void* buffer, size_t length) override;
    Task<expected<size_t, Error>> async_write_all(const void* buffer, size_t length) override;
    Task<expected<size_t, Error>> async_transmit_file(FileHandle file, size_t offset, size_t length) override;
    void close() override;
    bool is_open() const noexcept override;
    void set_timeout(std::chrono::milliseconds timeout) override;
    std::string remote_address() const override;
    uint16_t remote_port() const noexcept override;
    void set_cancellation_token(CancellationToken token) override;

    void on_datagram(std::vector<uint8_t> data);

private:
    UdpServer* server_;
    UdpEndpoint peer_endpoint_;
    std::queue<std::vector<uint8_t>> pending_datagrams_;
    std::coroutine_handle<> read_waiter_{nullptr};
    bool closed_{false};

    friend class UdpReadAwaiter;
};

class UdpServer {
public:
    UdpServer(IoContext& ctx);
    ~UdpServer();

    expected<void, Error> listen(uint16_t port);
    Task<expected<std::unique_ptr<Connection>, Error>> async_accept();

    // Runs the background receive loop
    Task<void> run();

    // Used by UdpConnection
    Task<expected<size_t, Error>> send_to(const void* buffer, size_t length, const UdpEndpoint& peer);
    IoContext& context();

    // Invoked by UdpConnection when it closes to remove it from the map
    void remove_connection(const UdpEndpoint& peer);

private:
    IoContext& ctx_;
    std::unique_ptr<UdpSocket> socket_;

    // endpoint string -> connection (non-owning)
    std::unordered_map<std::string, UdpConnection*> connections_;
    // Queue of newly accepted connections
    std::queue<std::unique_ptr<Connection>> new_connections_;
    std::coroutine_handle<> accept_waiter_{nullptr};
    bool stopped_{false};

    friend class UdpConnection;
    friend class UdpAcceptAwaiter;
};

} // namespace coroute::net
