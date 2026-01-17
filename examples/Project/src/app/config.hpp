#pragma once

#include <cstdint>
#include <string>
#include <filesystem>

namespace project {

struct Config {
    // Server settings
    uint16_t port = 8080;
    std::string host = "0.0.0.0";
    
    // Security
    std::string session_secret = "change-me-in-production";
    
    // Paths (relative to executable or absolute)
    std::filesystem::path static_dir;
    std::filesystem::path template_dir;
    
    // TLS (optional)
    bool enable_tls = false;
    std::filesystem::path cert_file;
    std::filesystem::path key_file;
    
    // Load configuration from environment variables
    static Config from_env();
    
    // Get the source directory (for development)
    static std::filesystem::path source_dir();
};

} // namespace project
