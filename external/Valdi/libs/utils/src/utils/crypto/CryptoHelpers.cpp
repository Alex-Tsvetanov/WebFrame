#include "utils/crypto/CryptoHelpers.hpp"

#include "utils/logging/Log.hpp"

#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <openssl/err.h>

namespace snap::utils::crypto {

void logCryptoErrors() {
#ifdef NDEBUG
    while (ERR_get_error() != 0u) {
    }
#else
    while (uint32_t error = ERR_get_error()) {
        std::array<char, 120> buf;
        ERR_error_string_n(error, buf.data(), buf.size());
        SCLOGE("[utils][CryptoHelpers]", "OpenSSL error: {}", buf.data());
    }
#endif
}

} // namespace snap::utils::crypto
