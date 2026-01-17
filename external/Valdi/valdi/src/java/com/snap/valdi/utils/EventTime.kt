package com.snap.valdi.utils

import com.snapchat.client.valdi.NativeBridge

class EventTime {
    companion object {
        @JvmStatic
        fun getTimeSeconds(): Double {
            return NativeBridge.getCurrentEventTime()
        }
    }
}
