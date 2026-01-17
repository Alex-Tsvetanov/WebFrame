package com.snap.valdi

import com.snap.valdi.utils.ValdiJNI
import com.snapchat.client.valdi.NativeBridge

class BuildOptions(val isDebug: Boolean,
                   val loggingEnabled: Boolean,
                   val tracingEnabled: Boolean,
                   val is64Bits: Boolean,
                   val snapDrawingAvailable: Boolean,
                   val lottieAvailable: Boolean) {

    override fun toString(): String {
        val addressing = if (is64Bits) "64bits" else "32bits"
        return "debug: $isDebug, loggingEnabled: $loggingEnabled, tracingEnabled: $tracingEnabled, addressing: $addressing"
    }

    companion object {
        val get by lazy {
            if (ValdiJNI.available) {
                val options = NativeBridge.getBuildOptions()
                val isDebug = (options and (1 shl 0)) != 0
                val loggingEnabled = (options and (1 shl 1)) != 0
                val tracingEnabled = (options and (1 shl 2)) != 0
                val is64Bits = (options and (1 shl 3)) != 0
                val snapDrawingAvailable = (options and (1 shl 4)) != 0
                val lottieAvailable = (options and (1 shl 5)) != 0
                BuildOptions(isDebug, loggingEnabled, tracingEnabled, is64Bits, snapDrawingAvailable, lottieAvailable)
            } else {
                BuildOptions(isDebug = true, loggingEnabled = true, tracingEnabled = true, is64Bits = false, snapDrawingAvailable = false, lottieAvailable = false)
            }
        }
    }
}