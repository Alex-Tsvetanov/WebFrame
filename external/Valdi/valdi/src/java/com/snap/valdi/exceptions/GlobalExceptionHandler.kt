package com.snap.valdi.exceptions

import androidx.annotation.Keep
import com.snap.valdi.BuildOptions
import java.util.concurrent.atomic.AtomicLong
import java.util.concurrent.atomic.AtomicReference

public interface HostUncaughtExceptionHandler {
    fun uncaughtException(thread: Thread, exception: Throwable)
}

class DefaultUncaughtExceptionHandler: HostUncaughtExceptionHandler {
    override fun uncaughtException(thread: Thread, exception: Throwable) {
        val handler = Thread.getDefaultUncaughtExceptionHandler()
        if (handler != null) {
            handler.uncaughtException(thread, exception)
        } else {
            println("[valdi] [fatal] Thread.getDefaultUncaughtExceptionHandler() returned null")
        }
    }
}

@Keep
class GlobalExceptionHandler {

    companion object {
        private val sleepTimeBeforeRethrowing: AtomicLong = AtomicLong(0)

        @JvmStatic
        @Keep
        fun setSleepTimeBeforeRethrowing(sleepTimeBeforeRethrowing: Long) {
            this.sleepTimeBeforeRethrowing.set(sleepTimeBeforeRethrowing)
        }

        private val hostUncaughtExceptionHandler = AtomicReference<HostUncaughtExceptionHandler>(null)

        fun setHostUncaughtExceptionHandler(hostUncaughtExceptionHandler: HostUncaughtExceptionHandler) {
            this.hostUncaughtExceptionHandler.set(hostUncaughtExceptionHandler)
        }

        @JvmStatic
        @Keep
        fun onFatalExceptionFromCPP(exception: Throwable, additionalInfo: String?): String {
            val errorMessage = printAndReturnFatalErrorMessage(exception, additionalInfo)

            var exceptionToRethrow = exception

            if (additionalInfo != null) {
                exceptionToRethrow = RuntimeException(additionalInfo, exception)
            }

            callDefaultUncaughtExceptionHandler(exceptionToRethrow)

            // We shouldn't need to do this, but if the crash leaks to the JNI layer,
            // we don't currently seem to have any way mechanism for printing the full
            // exception chain with all the causes.
            var abortMessage = errorMessage
            var cause = exception.cause
            while (cause != null) {
                abortMessage = "$abortMessage - cause: [$cause]"
                cause = cause.cause
            }
            return abortMessage
        }

        fun onFatalException(exception: Throwable): Nothing {
            printAndReturnFatalErrorMessage(exception, null)

            callDefaultUncaughtExceptionHandler(exception)

            throw exception
        }

        private fun printAndReturnFatalErrorMessage(exception: Throwable, additionalInfo: String?): String {
            for (i in 0 until 5) {
                println("========================================================")
            }
            var errorMessage = "FATAL UNMANAGED EXCEPTION THROWN: "
            if (additionalInfo != null) {
                errorMessage += "$additionalInfo "
            }

            errorMessage += "[$exception]"
            println("[valdi] $errorMessage")
            if (BuildOptions.get.isDebug) {
                exception.printStackTrace()
            }

            return errorMessage
        }
        private fun callDefaultUncaughtExceptionHandler(exception: Throwable) {
            var exceptionHandler = hostUncaughtExceptionHandler.get()
                ?: DefaultUncaughtExceptionHandler()
            exceptionHandler.uncaughtException(Thread.currentThread(), exception)

            val sleepTimeBeforeRethrowing = this.sleepTimeBeforeRethrowing.get()
            if (sleepTimeBeforeRethrowing > 0) {
                // Sleep for $sleepTimeBeforeRethrowing ms to give the Java crash
                // reporting machinery enough time to kick in.
                Thread.sleep(sleepTimeBeforeRethrowing);
            }
        }
    }

}
