#pragma once

#include <string>
#include <string_view>
#include <filesystem>

#include "coroute/net/io_context.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace coroute {

// ============================================================================
// Zero-Copy File Transfer Utilities
// ============================================================================

// RAII wrapper for file handles
class FileHandleGuard {
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
#endif

public:
    FileHandleGuard() = default;
    
    // Open file for reading
    explicit FileHandleGuard(const std::filesystem::path& path) {
#ifdef _WIN32
        handle_ = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
            nullptr
        );
#else
        fd_ = open(path.c_str(), O_RDONLY);
#endif
    }
    
    ~FileHandleGuard() {
        close();
    }
    
    // Non-copyable
    FileHandleGuard(const FileHandleGuard&) = delete;
    FileHandleGuard& operator=(const FileHandleGuard&) = delete;
    
    // Movable
    FileHandleGuard(FileHandleGuard&& other) noexcept {
#ifdef _WIN32
        handle_ = other.handle_;
        other.handle_ = INVALID_HANDLE_VALUE;
#else
        fd_ = other.fd_;
        other.fd_ = -1;
#endif
    }
    
    FileHandleGuard& operator=(FileHandleGuard&& other) noexcept {
        if (this != &other) {
            close();
#ifdef _WIN32
            handle_ = other.handle_;
            other.handle_ = INVALID_HANDLE_VALUE;
#else
            fd_ = other.fd_;
            other.fd_ = -1;
#endif
        }
        return *this;
    }
    
    void close() {
#ifdef _WIN32
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
#else
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
#endif
    }
    
    bool is_valid() const {
#ifdef _WIN32
        return handle_ != INVALID_HANDLE_VALUE;
#else
        return fd_ >= 0;
#endif
    }
    
    net::FileHandle get() const {
#ifdef _WIN32
        return static_cast<net::FileHandle>(handle_);
#else
        return fd_;
#endif
    }
    
    // Get file size
    size_t size() const {
#ifdef _WIN32
        LARGE_INTEGER li;
        if (GetFileSizeEx(handle_, &li)) {
            return static_cast<size_t>(li.QuadPart);
        }
        return 0;
#else
        struct stat st;
        if (fstat(fd_, &st) == 0) {
            return static_cast<size_t>(st.st_size);
        }
        return 0;
#endif
    }
};

// ============================================================================
// Zero-Copy Send Functions
// ============================================================================

// Send file using zero-copy (TransmitFile/sendfile)
// Returns bytes sent or error
inline Task<net::TransmitResult> send_file_zero_copy(
    net::Connection& conn,
    const std::filesystem::path& path,
    size_t offset = 0,
    size_t length = 0)  // 0 = entire file from offset
{
    FileHandleGuard file(path);
    if (!file.is_valid()) {
        co_return unexpected(Error::io(IoError::InvalidArgument, 
            "Failed to open file: " + path.string()));
    }
    
    if (length == 0) {
        length = file.size() - offset;
    }
    
    co_return co_await conn.async_transmit_file(file.get(), offset, length);
}

// Send response headers then file body using zero-copy
// Useful for static file serving
inline Task<expected<size_t, Error>> send_response_with_file(
    net::Connection& conn,
    Response& headers_response,
    const std::filesystem::path& file_path,
    size_t offset = 0,
    size_t length = 0)
{
    // Send headers first
    std::string headers = headers_response.serialize();
    auto write_result = co_await conn.async_write_all(headers.data(), headers.size());
    if (!write_result) {
        co_return unexpected(write_result.error());
    }
    
    // Then send file body using zero-copy
    auto transmit_result = co_await send_file_zero_copy(conn, file_path, offset, length);
    if (!transmit_result) {
        co_return unexpected(transmit_result.error());
    }
    
    co_return *write_result + *transmit_result;
}

} // namespace coroute
