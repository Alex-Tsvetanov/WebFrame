package com.snap.valdi.utils

import androidx.annotation.Keep

@Keep
class ValdiThread(name: String, val ptr: Long): Runnable {

    private val thread = Thread(this, name)

    var qosClass: QoSClass = QoSClass.NORMAL
        set(value) {
            field = value
            thread.priority = value.toThreadPriority()
        }

    fun start() {
        thread.start()
    }

    @Keep
    fun join() {
        try {
            thread.join()
        } catch (interrupted: InterruptedException) {}
    }

    @Keep
    fun updateQoS(qosClassValue: Int) {
        this.qosClass = QoSClass.fromValue(qosClassValue)
    }

    override fun run() {
        nativeThreadEntryPoint(ptr)
    }

    companion object {
        @Keep
        @JvmStatic
        fun start(name: String, qosClassValue: Int, ptr: Long): ValdiThread {
            val qosClass = QoSClass.fromValue(qosClassValue)
            val thread = ValdiThread(name, ptr)
            thread.qosClass = qosClass
            thread.start()
            return thread
        }

        @JvmStatic
        private external fun nativeThreadEntryPoint(ptr: Long)
    }

}
