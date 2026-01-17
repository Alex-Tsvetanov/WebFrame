#pragma once

namespace snap::utils::crypto {

/**
 * Logs the open SSL error stack for debug builds.
 * In release builds will clear the stack but not log anything.
 */
void logCryptoErrors();

} // namespace snap::utils::crypto
