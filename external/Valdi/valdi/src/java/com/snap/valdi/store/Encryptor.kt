package com.snap.valdi.store

import android.os.Build
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Base64
import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.exceptions.messageWithCauses
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.error
import com.snap.valdi.utils.info
import java.security.GeneralSecurityException
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

/**
 * The Encryptor allows to encrypt and decrypt data using a private
 * key that is created and managed by Android and stored on the device.
 */
class Encryptor(val logger: Logger, private val bypassEncryptor: Boolean = false) {

    private val ANDROID_KEY_STORE = "AndroidKeyStore"
    private val ALIAS = "Valdi"
    private val TRANSFORMATION = "AES/GCM/NoPadding"
    private val keyStore = KeyStore.getInstance("AndroidKeyStore")
    private val ivLength = 12
    private val secretKey: SecretKey?

    init {
        try {
            keyStore.load(null)
        } catch (exc: Exception) {
            logger.error("Failed to load AndroidKeyStore: ${exc.messageWithCauses()}")
        }
        secretKey = try {
            resolveSecretKey()
        } catch (exc: Exception) {
            logger.error("Failed to resolve SecretKey: ${exc.messageWithCauses()}")
            null
        }
    }

    fun encrypt(bytes: ByteArray): String {
        if (secretKey == null) {
            // Would happen on Android < 6, there was no support for hardware isolated security.
            return Base64.encodeToString(bytes, Base64.DEFAULT)
        }

        val cipher = Cipher.getInstance(TRANSFORMATION)

        cipher.init(Cipher.ENCRYPT_MODE, secretKey)

        val encryptedBytes = cipher.doFinal(bytes)

        val result = cipher.iv + encryptedBytes

        return Base64.encodeToString(result, Base64.DEFAULT)
    }

    fun decrypt(encryptedData: String): ByteArray? {
        try {
            val result = Base64.decode(encryptedData, Base64.DEFAULT)
            if (secretKey == null) {
                // Would happen on Android < 6, there was no support for hardware isolated security.
                return result
            }

            if (result.size < ivLength) {
                throw ValdiException("Invalid IV")
            }

            val cipher = Cipher.getInstance(TRANSFORMATION)

            val spec = GCMParameterSpec(128, result.copyOfRange(0, ivLength))
            cipher.init(Cipher.DECRYPT_MODE, secretKey, spec)

            return cipher.doFinal(result, ivLength, result.size - ivLength)
        } catch (exc: Exception) {
            logger.error("Failed to decrypt data: ${exc.message}")
            return null
        }
    }

    private fun resolveSecretKey(): SecretKey? {
        if (bypassEncryptor || Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            // KeyGenParameterSpec is not supported below Android M
            return null
        }

        var secretKey = getSecretKey()
        if (secretKey != null) {
            logger.info("Restored Encryptor SecretKey")
            return secretKey
        }

        val keyGenerator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, ANDROID_KEY_STORE)
        keyGenerator?.init(KeyGenParameterSpec.Builder(ALIAS, KeyProperties.PURPOSE_ENCRYPT
                or KeyProperties.PURPOSE_DECRYPT)
                .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                .build())

        secretKey = keyGenerator?.generateKey()

        if (secretKey != null) {
            logger.info("Generated Encryptor SecretKey")
        } else {
            logger.info("Failed to generated Encryptor SecretKey")
        }

        return secretKey
    }

    private fun getSecretKey(): SecretKey? {
        return try {
            val entry = keyStore.getEntry(ALIAS, null) as? KeyStore.SecretKeyEntry
            entry?.secretKey
        } catch (exc: GeneralSecurityException) {
            null
        }
    }


}
