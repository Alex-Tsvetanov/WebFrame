package com.snap.valdi.utils

import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.exceptions.GlobalExceptionHandler
import com.snap.valdi.exceptions.messageWithCauses

object BridgeCallUtils {
    @JvmStatic
    fun <T> handleCall(marshallerHandle: Long, cb: () -> T): T? {
        return try {
            return cb()
        } catch (exc: ValdiException) {
            ValdiMarshallerCPP.javaEntry(marshallerHandle) {
                it.setError(exc.messageWithCauses())
                false
            }
            null
        } catch (exc: Throwable) {
            GlobalExceptionHandler.onFatalException(exc)
            null
        }
    }
}
