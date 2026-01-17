package com.snap.valdi.store

import android.content.Context
import android.content.SharedPreferences
import androidx.annotation.Keep
import com.snap.valdi.exceptions.GlobalExceptionHandler
import com.snap.valdi.exceptions.messageWithCauses
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.error
import com.snap.valdi.utils.trace
import javax.crypto.KeyGenerator

/**
 * KeychainUtils is a utility implementation file leveraged by AndroidKeychain at the JNI level.
 * It allows the JNI implementation to manipulate the SharedPreferences and retrieve a private
 * secret key that is encoded into the device secure enclave.
 *
 * The private key that is returned to C++ is generated at the Java level and stored in the SharedPreferences
 * so that it can be retrieved later. Before being stored into the SharedPreferences, the private key
 * is encrypted by the Encryptor class which uses the device secure enclave.
 */
class KeychainUtils(val context: Context, private val bypassEncryptor: Boolean, val logger: Logger) {

    private val PRIVATE_KEY = "__PRIVATE_KEY__"
    private val USE_ENCRYPTOR = "__USE_ENCRYPTOR__"

    private val secretKey = lazy { resolveSecretKey() }

    private fun getSharedPreferences(): SharedPreferences {
        return checkException {
            context.getSharedPreferences("ValdiKeychain", Context.MODE_PRIVATE)
        }
    }

    private fun generateSecretKey(encryptor: Encryptor): ByteArray {
        val keyGenerator = KeyGenerator.getInstance("AES")
        keyGenerator.init(128)
        val secretKey = keyGenerator.generateKey().encoded

        val encryptedSecretKey = encryptor.encrypt(secretKey)

        // TODO(yhuang6): Investigate possible failure state where encryptor tries to decrypt
        // with a different key than previously stored. This causes key size mismatches in C++ level
        checkException {
            getSharedPreferences()
                    .edit()
                    .clear()
                    .putString(PRIVATE_KEY, encryptedSecretKey)
                    .putBoolean(USE_ENCRYPTOR, bypassEncryptor)
                    .apply()
        }

        return secretKey
    }

    private fun doResolveSecretKey(): ByteArray {
        val encryptor = Encryptor(logger, bypassEncryptor)
        val useNativeAppStorage = doGetBoolean(USE_ENCRYPTOR)

        // When we bypass, we need to check if a previous key exists and reinitialize with new
        // scheme if it detects that storage format has changed.
        if (bypassEncryptor != useNativeAppStorage) {
            return generateSecretKey(encryptor)
        }

        val key = doGet(PRIVATE_KEY) ?: return generateSecretKey(encryptor)

        return encryptor.decrypt(key) ?: return generateSecretKey(encryptor)
    }

    private fun wait(minTimeMs: Long, maxTimeMs: Long) {
        Thread.sleep((minTimeMs..maxTimeMs).random(), 0)
    }

    private fun resolveSecretKey(): ByteArray {
        trace({"Valdi.resolveKeychainSecretKey"}) {
            // Do 3 attempts to work around an issue with the Android Keychain
            try {
                return doResolveSecretKey()
            } catch (exc: Exception) {
                logger.error("Failed to resolve SecretKey (attempt #1): ${exc.messageWithCauses()}")
            }
            wait(100L, 200L)
            try {
                return doResolveSecretKey()
            } catch (exc: Exception) {
                logger.error("Failed to resolve SecretKey (attempt #2): ${exc.messageWithCauses()}")
            }
            wait(300L, 600L)
            return doResolveSecretKey()
        }
    }

    @Keep
    fun getSecretKey(): ByteArray {
        return secretKey.value
    }

    private fun doGet(key: String): String? {
        return checkException {
            getSharedPreferences().getString(key, null)
        }
    }

    private fun doGetBoolean(key: String): Boolean {
        return checkException {
            getSharedPreferences().getBoolean(key, false)
        }
    }

    private fun getItemKey(key: String): String {
        return "item.$key"
    }

    @Keep
    fun get(key: String): String? {
        val itemKey = getItemKey(key)
        return doGet(itemKey)
    }

    @Keep
    fun set(key: String, value: String): String? {
        val itemKey = getItemKey(key)
        return try {
            getSharedPreferences().edit().putString(itemKey, value).apply()
            null
        } catch (exc: Exception) {
            exc.messageWithCauses()
        }
    }

    @Keep
    fun erase(key: String): Boolean {
        val itemKey = getItemKey(key)
        return try {
            getSharedPreferences().edit().remove(itemKey).apply()
            true
        } catch (exc: Exception) {
            false
        }
    }

    private inline fun <T>checkException(crossinline block:  () -> T): T {
        try {
            return block()
        } catch (throwable: Throwable) {
            GlobalExceptionHandler.onFatalException(throwable)
        }
    }

}
