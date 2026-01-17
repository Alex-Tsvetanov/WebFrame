#include <catch2/catch_test_macros.hpp>
#include <coroute/net/connection_pool.hpp>
#include <coroute/coro/task.hpp>
#include <coroute/core/error.hpp>

using namespace coroute;
using namespace coroute::net;

// Mock connection for testing
class MockConnection : public Connection {
    bool open_ = true;
    int id_;
    static int next_id_;
    
public:
    MockConnection() : id_(next_id_++) {}
    
    int id() const { return id_; }
    
    Task<ReadResult> async_read(void*, size_t) override {
        co_return coroute::unexpected(coroute::Error::io(coroute::IoError::InvalidArgument, "Mock"));
    }
    
    Task<ReadResult> async_read_until(void*, size_t, char) override {
        co_return coroute::unexpected(coroute::Error::io(coroute::IoError::InvalidArgument, "Mock"));
    }
    
    Task<WriteResult> async_write(const void*, size_t) override {
        co_return coroute::unexpected(coroute::Error::io(coroute::IoError::InvalidArgument, "Mock"));
    }
    
    Task<WriteResult> async_write_all(const void*, size_t) override {
        co_return coroute::unexpected(coroute::Error::io(coroute::IoError::InvalidArgument, "Mock"));
    }
    
    Task<TransmitResult> async_transmit_file(FileHandle, size_t, size_t) override {
        co_return coroute::unexpected(coroute::Error::io(coroute::IoError::InvalidArgument, "Mock"));
    }
    
    void close() override { open_ = false; }
    bool is_open() const noexcept override { return open_; }
    void set_timeout(std::chrono::milliseconds) override {}
    std::string remote_address() const override { return "127.0.0.1"; }
    uint16_t remote_port() const noexcept override { return 12345; }
    void set_cancellation_token(coroute::CancellationToken) override {}
};

int MockConnection::next_id_ = 0;

TEST_CASE("ConnectionPool basic operations", "[connection_pool]") {
    ConnectionPool pool;
    
    SECTION("Acquire returns nullptr when empty") {
        auto conn = pool.acquire();
        CHECK(conn == nullptr);
    }
    
    SECTION("Release and acquire reuses connection") {
        auto conn1 = std::make_unique<MockConnection>();
        int id = static_cast<MockConnection*>(conn1.get())->id();
        
        pool.release(std::move(conn1));
        CHECK(pool.pool_size() == 1);
        
        auto conn2 = pool.acquire();
        REQUIRE(conn2 != nullptr);
        CHECK(static_cast<MockConnection*>(conn2.get())->id() == id);
        CHECK(pool.pool_size() == 0);
    }
    
    SECTION("Statistics are tracked") {
        pool.release(std::make_unique<MockConnection>());
        pool.release(std::make_unique<MockConnection>());
        
        CHECK(pool.total_acquired() == 0);
        CHECK(pool.total_reused() == 0);
        
        auto conn1 = pool.acquire();
        CHECK(pool.total_acquired() == 1);
        CHECK(pool.total_reused() == 1);
        
        auto conn2 = pool.acquire();
        CHECK(pool.total_acquired() == 2);
        CHECK(pool.total_reused() == 2);
        
        // Empty pool
        auto conn3 = pool.acquire();
        CHECK(conn3 == nullptr);
        CHECK(pool.total_acquired() == 3);
        CHECK(pool.total_reused() == 2);  // Not reused
    }
}

TEST_CASE("ConnectionPool max size", "[connection_pool]") {
    ConnectionPoolConfig config;
    config.max_size = 2;
    ConnectionPool pool(config);
    
    pool.release(std::make_unique<MockConnection>());
    pool.release(std::make_unique<MockConnection>());
    pool.release(std::make_unique<MockConnection>());  // Should be discarded
    
    CHECK(pool.pool_size() == 2);
}

TEST_CASE("ConnectionPool resetter", "[connection_pool]") {
    ConnectionPool pool;
    bool reset_called = false;
    
    pool.set_resetter([&](Connection&) {
        reset_called = true;
    });
    
    pool.release(std::make_unique<MockConnection>());
    CHECK(reset_called);
}

TEST_CASE("PooledConnection RAII", "[connection_pool]") {
    ConnectionPool pool;
    
    SECTION("Returns to pool on destruction") {
        {
            auto conn = std::make_unique<MockConnection>();
            PooledConnection pooled(std::move(conn), &pool);
        }
        CHECK(pool.pool_size() == 1);
    }
    
    SECTION("Release prevents return to pool") {
        {
            auto conn = std::make_unique<MockConnection>();
            PooledConnection pooled(std::move(conn), &pool);
            auto released = pooled.release();
            CHECK(released != nullptr);
        }
        CHECK(pool.pool_size() == 0);
    }
    
    SECTION("Detach keeps connection but doesn't return") {
        {
            auto conn = std::make_unique<MockConnection>();
            PooledConnection pooled(std::move(conn), &pool);
            pooled.detach();
            // Connection still valid but won't return to pool
        }
        CHECK(pool.pool_size() == 0);
    }
    
    SECTION("Move semantics") {
        auto conn = std::make_unique<MockConnection>();
        int id = static_cast<MockConnection*>(conn.get())->id();
        
        PooledConnection pooled1(std::move(conn), &pool);
        PooledConnection pooled2 = std::move(pooled1);
        
        CHECK(!pooled1);
        CHECK(pooled2);
        CHECK(static_cast<MockConnection*>(pooled2.get())->id() == id);
    }
}

TEST_CASE("ConnectionPool reuse rate", "[connection_pool]") {
    ConnectionPool pool;
    
    CHECK(pool.reuse_rate() == 0.0);
    
    // Add some connections
    pool.release(std::make_unique<MockConnection>());
    pool.release(std::make_unique<MockConnection>());
    
    // Acquire 4 times (2 reused, 2 not)
    auto c1 = pool.acquire();  // reused
    auto c2 = pool.acquire();  // reused
    auto c3 = pool.acquire();  // not reused (nullptr)
    auto c4 = pool.acquire();  // not reused (nullptr)
    
    CHECK(pool.total_acquired() == 4);
    CHECK(pool.total_reused() == 2);
    CHECK(pool.reuse_rate() == 0.5);
}

TEST_CASE("ConnectionPool populate", "[connection_pool]") {
    ConnectionPool pool;
    
    pool.populate([]() { return std::make_unique<MockConnection>(); }, 5);
    
    CHECK(pool.pool_size() == 5);
    
    // Acquire all
    for (int i = 0; i < 5; ++i) {
        auto conn = pool.acquire();
        CHECK(conn != nullptr);
    }
    
    CHECK(pool.pool_size() == 0);
}

TEST_CASE("ConnectionPool disable reuse", "[connection_pool]") {
    ConnectionPoolConfig config;
    config.enable_reuse = false;
    ConnectionPool pool(config);
    
    pool.release(std::make_unique<MockConnection>());
    
    // Should not be added to pool
    CHECK(pool.pool_size() == 0);
}
