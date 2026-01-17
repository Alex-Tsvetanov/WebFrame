#include "config.hpp"
#include <cstdlib>

namespace project {

std::filesystem::path Config::source_dir() {
    // __FILE__ gives us the path to this source file at compile time
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

Config Config::from_env() {
    Config config;
    
    // Server settings
    if (const char* port = std::getenv("PORT")) {
        config.port = static_cast<uint16_t>(std::stoi(port));
    }
    if (const char* host = std::getenv("HOST")) {
        config.host = host;
    }
    
    // Security
    if (const char* secret = std::getenv("SESSION_SECRET")) {
        config.session_secret = secret;
    }
    
    // Paths - default to source directory for development
    auto src_dir = source_dir();
    config.static_dir = src_dir / "static";
    config.template_dir = src_dir / "templates";
    
    if (const char* static_path = std::getenv("STATIC_DIR")) {
        config.static_dir = static_path;
    }
    if (const char* template_path = std::getenv("TEMPLATE_DIR")) {
        config.template_dir = template_path;
    }
    
    // TLS
    if (const char* tls = std::getenv("ENABLE_TLS")) {
        config.enable_tls = (std::string(tls) == "1" || std::string(tls) == "true");
    }
    if (const char* cert = std::getenv("TLS_CERT")) {
        config.cert_file = cert;
    }
    if (const char* key = std::getenv("TLS_KEY")) {
        config.key_file = key;
    }
    
    return config;
}

} // namespace project
