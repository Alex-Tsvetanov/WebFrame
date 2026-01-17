#pragma once

#ifdef COROUTE_HAS_TLS

#include <memory>
#include <string>
#include <string_view>
#include <filesystem>
#include <functional>
#include <optional>

#include "coroute/net/io_context.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"

// Forward declare OpenSSL types to avoid header pollution
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace coroute::net {

// ============================================================================
// TLS Configuration
// ============================================================================

struct TlsConfig {
    // Certificate and key paths
    std::filesystem::path cert_file;
    std::filesystem::path key_file;
    
    // Optional CA certificate for client verification
    std::filesystem::path ca_file;
    
    // Optional certificate chain
    std::filesystem::path chain_file;
    
    // Minimum TLS version (default: TLS 1.2)
    enum class MinVersion { TLS_1_2, TLS_1_3 } min_version = MinVersion::TLS_1_2;
    
    // Enable client certificate verification
    bool verify_client = false;
    
    // Cipher suites (empty = use defaults)
    std::string ciphers;
    
    // ALPN protocols (e.g., "h2", "http/1.1")
    std::vector<std::string> alpn_protocols;
    
    // Session ticket support
    bool session_tickets = true;
    
    // Session cache size (0 = disabled)
    size_t session_cache_size = 20480;
};

// ============================================================================
// TLS Context - Manages SSL_CTX
// ============================================================================

class TlsContext {
public:
    ~TlsContext();
    
    // Non-copyable
    TlsContext(const TlsContext&) = delete;
    TlsContext& operator=(const TlsContext&) = delete;
    
    // Movable
    TlsContext(TlsContext&& other) noexcept;
    TlsContext& operator=(TlsContext&& other) noexcept;
    
    // Create TLS context from configuration
    static expected<TlsContext, Error> create(const TlsConfig& config);
    
    // Get the negotiated ALPN protocol after handshake
    std::optional<std::string_view> alpn_protocol(SSL* ssl) const;
    
    // Access underlying context (for advanced use)
    SSL_CTX* native_handle() const noexcept { return ctx_; }
    
    // SNI callback type
    using SniCallback = std::function<TlsContext*(const std::string& hostname)>;
    
    // Set SNI callback for virtual hosting
    void set_sni_callback(SniCallback callback);

private:
    TlsContext() = default;
    SSL_CTX* ctx_ = nullptr;
    SniCallback sni_callback_;
};

// ============================================================================
// TLS Connection - Wraps a Connection with TLS
// ============================================================================

class TlsConnection : public Connection {
public:
    ~TlsConnection() override;
    
    // Create TLS connection wrapping an existing connection
    static expected<std::unique_ptr<TlsConnection>, Error> create(
        std::unique_ptr<Connection> inner,
        TlsContext& ctx,
        bool is_server = true
    );
    
    // Perform TLS handshake
    Task<expected<void, Error>> handshake();
    
    // Connection interface
    Task<ReadResult> async_read(void* buffer, size_t len) override;
    Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override;
    Task<WriteResult> async_write(const void* data, size_t len) override;
    Task<WriteResult> async_write_all(const void* data, size_t len) override;
    Task<TransmitResult> async_transmit_file(
        FileHandle file, size_t offset, size_t length) override;
    void close() override;
    bool is_open() const noexcept override;
    void set_timeout(std::chrono::milliseconds timeout) override;
    std::string remote_address() const override;
    uint16_t remote_port() const noexcept override;
    void set_cancellation_token(CancellationToken token) override;
    
    // Get peer certificate info (if client cert verification enabled)
    struct PeerCertInfo {
        std::string subject;
        std::string issuer;
        std::string serial;
    };
    std::optional<PeerCertInfo> peer_certificate() const;
    
    // Get negotiated protocol (from ALPN)
    std::optional<std::string_view> negotiated_protocol() const;
    
    // Get TLS version string
    std::string_view tls_version() const;

private:
    TlsConnection() = default;
    
    std::unique_ptr<Connection> inner_;
    SSL* ssl_ = nullptr;
    TlsContext* ctx_ = nullptr;
    
    // BIO pair for async I/O (using void* to avoid including OpenSSL headers)
    void* rbio_ = nullptr;  // Read BIO (network -> SSL)
    void* wbio_ = nullptr;  // Write BIO (SSL -> network)
    
    // Internal helpers
    Task<expected<void, Error>> flush_write_bio();
    Task<expected<void, Error>> fill_read_bio();
};

// ============================================================================
// TLS Listener - Accepts TLS connections
// ============================================================================

class TlsListener {
public:
    TlsListener(std::unique_ptr<Listener> inner, TlsContext& ctx);
    ~TlsListener();
    
    // Accept a new TLS connection (performs handshake)
    Task<expected<std::unique_ptr<TlsConnection>, Error>> accept();
    
    // Accept without handshake (caller must call handshake())
    Task<expected<std::unique_ptr<TlsConnection>, Error>> accept_no_handshake();
    
    // Close the listener
    void close();

private:
    std::unique_ptr<Listener> inner_;
    TlsContext* ctx_;
};

} // namespace coroute::net

#endif // coroute_HAS_TLS
