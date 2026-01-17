#include "coroute/net/tls.hpp"

#ifdef COROUTE_HAS_TLS

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <cstring>

namespace coroute::net {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

std::string get_openssl_error() {
    unsigned long err = ERR_get_error();
    if (err == 0) return "Unknown SSL error";
    
    char buf[256];
    ERR_error_string_n(err, buf, sizeof(buf));
    return std::string(buf);
}

Error make_tls_error(const std::string& msg) {
    return Error::io(IoError::Unknown, msg + ": " + get_openssl_error());
}

// ALPN callback for server
int alpn_select_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen,
                         const unsigned char* in, unsigned int inlen, void* arg) {
    auto* config_protocols = static_cast<std::vector<std::string>*>(arg);
    if (!config_protocols || config_protocols->empty()) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    
    // Parse client protocols
    const unsigned char* client = in;
    const unsigned char* client_end = in + inlen;
    
    while (client < client_end) {
        unsigned char len = *client++;
        if (client + len > client_end) break;
        
        std::string_view client_proto(reinterpret_cast<const char*>(client), len);
        
        // Check if we support this protocol
        for (const auto& server_proto : *config_protocols) {
            if (client_proto == server_proto) {
                *out = client;
                *outlen = len;
                return SSL_TLSEXT_ERR_OK;
            }
        }
        
        client += len;
    }
    
    return SSL_TLSEXT_ERR_NOACK;
}

// SNI callback wrapper
int sni_callback(SSL* ssl, int* al, void* arg) {
    auto* ctx = static_cast<TlsContext*>(arg);
    const char* servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    
    if (servername && ctx) {
        // TODO: Implement SNI callback dispatch
        // For now, just accept any hostname
    }
    
    return SSL_TLSEXT_ERR_OK;
}

} // anonymous namespace

// ============================================================================
// TlsContext Implementation
// ============================================================================

TlsContext::~TlsContext() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
    }
}

TlsContext::TlsContext(TlsContext&& other) noexcept
    : ctx_(other.ctx_)
    , sni_callback_(std::move(other.sni_callback_))
{
    other.ctx_ = nullptr;
}

TlsContext& TlsContext::operator=(TlsContext&& other) noexcept {
    if (this != &other) {
        if (ctx_) SSL_CTX_free(ctx_);
        ctx_ = other.ctx_;
        sni_callback_ = std::move(other.sni_callback_);
        other.ctx_ = nullptr;
    }
    return *this;
}

expected<TlsContext, Error> TlsContext::create(const TlsConfig& config) {
    // Initialize OpenSSL (safe to call multiple times)
    static bool initialized = []() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        return true;
    }();
    (void)initialized;
    
    TlsContext result;
    
    // Create SSL context
    const SSL_METHOD* method = TLS_server_method();
    result.ctx_ = SSL_CTX_new(method);
    if (!result.ctx_) {
        return unexpected(make_tls_error("Failed to create SSL context"));
    }
    
    // Set minimum TLS version
    int min_version = (config.min_version == TlsConfig::MinVersion::TLS_1_3) 
        ? TLS1_3_VERSION : TLS1_2_VERSION;
    SSL_CTX_set_min_proto_version(result.ctx_, min_version);
    
    // Load certificate
    if (!config.cert_file.empty()) {
        if (SSL_CTX_use_certificate_file(result.ctx_, 
                config.cert_file.string().c_str(), SSL_FILETYPE_PEM) != 1) {
            return unexpected(make_tls_error("Failed to load certificate"));
        }
    }
    
    // Load certificate chain
    if (!config.chain_file.empty()) {
        if (SSL_CTX_use_certificate_chain_file(result.ctx_, 
                config.chain_file.string().c_str()) != 1) {
            return unexpected(make_tls_error("Failed to load certificate chain"));
        }
    }
    
    // Load private key
    if (!config.key_file.empty()) {
        if (SSL_CTX_use_PrivateKey_file(result.ctx_, 
                config.key_file.string().c_str(), SSL_FILETYPE_PEM) != 1) {
            return unexpected(make_tls_error("Failed to load private key"));
        }
        
        // Verify key matches certificate
        if (SSL_CTX_check_private_key(result.ctx_) != 1) {
            return unexpected(make_tls_error("Private key does not match certificate"));
        }
    }
    
    // Load CA certificate for client verification
    if (!config.ca_file.empty()) {
        if (SSL_CTX_load_verify_locations(result.ctx_, 
                config.ca_file.string().c_str(), nullptr) != 1) {
            return unexpected(make_tls_error("Failed to load CA certificate"));
        }
    }
    
    // Configure client verification
    if (config.verify_client) {
        SSL_CTX_set_verify(result.ctx_, 
            SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    }
    
    // Set cipher suites
    if (!config.ciphers.empty()) {
        if (SSL_CTX_set_cipher_list(result.ctx_, config.ciphers.c_str()) != 1) {
            return unexpected(make_tls_error("Failed to set cipher list"));
        }
    }
    
    // Configure session tickets
    if (!config.session_tickets) {
        SSL_CTX_set_options(result.ctx_, SSL_OP_NO_TICKET);
    }
    
    // Configure session cache
    if (config.session_cache_size > 0) {
        SSL_CTX_set_session_cache_mode(result.ctx_, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(result.ctx_, config.session_cache_size);
    } else {
        SSL_CTX_set_session_cache_mode(result.ctx_, SSL_SESS_CACHE_OFF);
    }
    
    // Set ALPN protocols
    if (!config.alpn_protocols.empty()) {
        // Store protocols in context for callback
        auto* protocols = new std::vector<std::string>(config.alpn_protocols);
        SSL_CTX_set_alpn_select_cb(result.ctx_, alpn_select_callback, protocols);
    }
    
    // Enable SNI
    SSL_CTX_set_tlsext_servername_callback(result.ctx_, sni_callback);
    SSL_CTX_set_tlsext_servername_arg(result.ctx_, &result);
    
    return result;
}

std::optional<std::string_view> TlsContext::alpn_protocol(SSL* ssl) const {
    const unsigned char* data = nullptr;
    unsigned int len = 0;
    SSL_get0_alpn_selected(ssl, &data, &len);
    if (data && len > 0) {
        return std::string_view(reinterpret_cast<const char*>(data), len);
    }
    return std::nullopt;
}

void TlsContext::set_sni_callback(SniCallback callback) {
    sni_callback_ = std::move(callback);
}

// ============================================================================
// TlsConnection Implementation
// ============================================================================

TlsConnection::~TlsConnection() {
    if (ssl_) {
        SSL_free(ssl_);  // This also frees the BIOs
    }
}

expected<std::unique_ptr<TlsConnection>, Error> TlsConnection::create(
    std::unique_ptr<Connection> inner,
    TlsContext& ctx,
    bool is_server)
{
    auto conn = std::unique_ptr<TlsConnection>(new TlsConnection());
    conn->inner_ = std::move(inner);
    conn->ctx_ = &ctx;
    
    // Create SSL object
    conn->ssl_ = SSL_new(ctx.native_handle());
    if (!conn->ssl_) {
        return unexpected(make_tls_error("Failed to create SSL object"));
    }
    
    // Create BIO pair for async I/O
    // rbio: we write network data into it, SSL reads from it
    // wbio: SSL writes encrypted data into it, we read from it
    BIO* rbio = BIO_new(BIO_s_mem());
    BIO* wbio = BIO_new(BIO_s_mem());
    conn->rbio_ = rbio;
    conn->wbio_ = wbio;
    
    if (!rbio || !wbio) {
        return unexpected(make_tls_error("Failed to create BIO pair"));
    }
    
    // Set BIOs (SSL takes ownership)
    SSL_set_bio(conn->ssl_, rbio, wbio);
    
    // Set server/client mode
    if (is_server) {
        SSL_set_accept_state(conn->ssl_);
    } else {
        SSL_set_connect_state(conn->ssl_);
    }
    
    return conn;
}

Task<expected<void, Error>> TlsConnection::handshake() {
    int iteration = 0;
    
    while (true) {
        iteration++;
        int result = SSL_do_handshake(ssl_);
        
        if (result == 1) {
            co_return expected<void, Error>{};
        }
        
        int err = SSL_get_error(ssl_, result);
        
        if (err == SSL_ERROR_WANT_READ) {
            auto flush_result = co_await flush_write_bio();
            if (!flush_result) co_return unexpected(flush_result.error());
            
            auto fill_result = co_await fill_read_bio();
            if (!fill_result) co_return unexpected(fill_result.error());
        }
        else if (err == SSL_ERROR_WANT_WRITE) {
            auto flush_result = co_await flush_write_bio();
            if (!flush_result) co_return unexpected(flush_result.error());
        }
        else {
            co_return unexpected(Error::io(IoError::Unknown, "TLS handshake failed: " + get_openssl_error()));
        }
        
        if (iteration > 100) {
            co_return unexpected(Error::io(IoError::Unknown, "TLS handshake timeout"));
        }
    }
}

Task<ReadResult> TlsConnection::async_read(void* buffer, size_t len) {
    while (true) {
        int result = SSL_read(ssl_, buffer, static_cast<int>(len));
        
        if (result > 0) {
            co_return static_cast<size_t>(result);
        }
        
        int err = SSL_get_error(ssl_, result);
        
        if (err == SSL_ERROR_WANT_READ) {
            // Need more data from network
            auto fill_result = co_await fill_read_bio();
            if (!fill_result) co_return unexpected(fill_result.error());
        }
        else if (err == SSL_ERROR_ZERO_RETURN) {
            // Clean shutdown
            co_return 0;
        }
        else {
            co_return unexpected(Error::io(IoError::Unknown, "TLS read error"));
        }
    }
}

Task<ReadResult> TlsConnection::async_read_until(void* buffer, size_t len, char delimiter) {
    // Simple implementation - read byte by byte until delimiter
    char* buf = static_cast<char*>(buffer);
    size_t total = 0;
    
    while (total < len) {
        auto result = co_await async_read(buf + total, 1);
        if (!result) co_return unexpected(result.error());
        if (*result == 0) break;
        total += *result;
        if (buf[total - 1] == delimiter) break;
    }
    
    co_return total;
}

Task<WriteResult> TlsConnection::async_write(const void* data, size_t len) {
    int result = SSL_write(ssl_, data, static_cast<int>(len));
    
    if (result > 0) {
        // Flush encrypted data to network
        auto flush_result = co_await flush_write_bio();
        if (!flush_result) co_return unexpected(flush_result.error());
        co_return static_cast<size_t>(result);
    }
    
    int err = SSL_get_error(ssl_, result);
    if (err == SSL_ERROR_WANT_WRITE) {
        auto flush_result = co_await flush_write_bio();
        if (!flush_result) co_return unexpected(flush_result.error());
        // Retry
        co_return co_await async_write(data, len);
    }
    
    co_return unexpected(Error::io(IoError::Unknown, "TLS write error"));
}

Task<WriteResult> TlsConnection::async_write_all(const void* data, size_t len) {
    const char* ptr = static_cast<const char*>(data);
    size_t remaining = len;
    size_t total = 0;
    
    while (remaining > 0) {
        auto result = co_await async_write(ptr, remaining);
        if (!result) co_return unexpected(result.error());
        if (*result == 0) co_return unexpected(Error::io(IoError::Unknown, "Connection closed during write"));
        
        ptr += *result;
        remaining -= *result;
        total += *result;
    }
    
    co_return total;
}

Task<TransmitResult> TlsConnection::async_transmit_file(
    FileHandle file, size_t offset, size_t length)
{
    // TLS doesn't support zero-copy file transmission
    // Fall back to reading file and writing through TLS
    // This is a simplified implementation - production code would buffer
    
    (void)file;
    (void)offset;
    (void)length;
    
    // For now, return error - caller should read file and use async_write
    co_return unexpected(Error::io(IoError::InvalidArgument, "TLS does not support zero-copy file transfer"));
}

void TlsConnection::close() {
    if (ssl_ && is_open()) {
        // Attempt clean shutdown (non-blocking)
        SSL_shutdown(ssl_);
    }
    if (inner_) {
        inner_->close();
    }
}

bool TlsConnection::is_open() const noexcept {
    return inner_ && inner_->is_open() && ssl_ && !SSL_get_shutdown(ssl_);
}

std::optional<TlsConnection::PeerCertInfo> TlsConnection::peer_certificate() const {
    X509* cert = SSL_get_peer_certificate(ssl_);
    if (!cert) return std::nullopt;
    
    PeerCertInfo info;
    
    // Get subject
    char* subject = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
    if (subject) {
        info.subject = subject;
        OPENSSL_free(subject);
    }
    
    // Get issuer
    char* issuer = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
    if (issuer) {
        info.issuer = issuer;
        OPENSSL_free(issuer);
    }
    
    // Get serial
    ASN1_INTEGER* serial = X509_get_serialNumber(cert);
    if (serial) {
        BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
        if (bn) {
            char* hex = BN_bn2hex(bn);
            if (hex) {
                info.serial = hex;
                OPENSSL_free(hex);
            }
            BN_free(bn);
        }
    }
    
    X509_free(cert);
    return info;
}

std::optional<std::string_view> TlsConnection::negotiated_protocol() const {
    return ctx_->alpn_protocol(ssl_);
}

std::string_view TlsConnection::tls_version() const {
    return SSL_get_version(ssl_);
}

void TlsConnection::set_timeout(std::chrono::milliseconds timeout) {
    if (inner_) inner_->set_timeout(timeout);
}

std::string TlsConnection::remote_address() const {
    return inner_ ? inner_->remote_address() : "";
}

uint16_t TlsConnection::remote_port() const noexcept {
    return inner_ ? inner_->remote_port() : 0;
}

void TlsConnection::set_cancellation_token(CancellationToken token) {
    if (inner_) inner_->set_cancellation_token(std::move(token));
}

Task<expected<void, Error>> TlsConnection::flush_write_bio() {
    char buf[16384];
    int pending;
    BIO* wbio = static_cast<BIO*>(wbio_);
    
    while ((pending = BIO_pending(wbio)) > 0) {
        int to_read = std::min(pending, static_cast<int>(sizeof(buf)));
        int read = BIO_read(wbio, buf, to_read);
        
        if (read > 0) {
            auto result = co_await inner_->async_write_all(buf, read);
            if (!result) co_return unexpected(result.error());
        }
    }
    
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> TlsConnection::fill_read_bio() {
    char buf[16384];
    BIO* rbio = static_cast<BIO*>(rbio_);
    
    auto result = co_await inner_->async_read(buf, sizeof(buf));
    if (!result) co_return unexpected(result.error());
    if (*result == 0) co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
    
    BIO_write(rbio, buf, static_cast<int>(*result));
    
    co_return expected<void, Error>{};
}

// ============================================================================
// TlsListener Implementation
// ============================================================================

TlsListener::TlsListener(std::unique_ptr<Listener> inner, TlsContext& ctx)
    : inner_(std::move(inner))
    , ctx_(&ctx)
{}

TlsListener::~TlsListener() = default;

Task<expected<std::unique_ptr<TlsConnection>, Error>> TlsListener::accept() {
    auto conn = co_await accept_no_handshake();
    if (!conn) co_return unexpected(conn.error());
    
    auto handshake_result = co_await (*conn)->handshake();
    if (!handshake_result) co_return unexpected(handshake_result.error());
    
    co_return std::move(*conn);
}

Task<expected<std::unique_ptr<TlsConnection>, Error>> TlsListener::accept_no_handshake() {
    auto inner_conn = co_await inner_->async_accept();
    if (!inner_conn) co_return unexpected(inner_conn.error());
    
    auto tls_conn = TlsConnection::create(std::move(*inner_conn), *ctx_, true);
    if (!tls_conn) co_return unexpected(tls_conn.error());
    
    co_return std::move(*tls_conn);
}

void TlsListener::close() {
    if (inner_) {
        inner_->close();
    }
}

} // namespace coroute::net

#endif // coroute_HAS_TLS
