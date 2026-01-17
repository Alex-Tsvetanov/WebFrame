package com.snap.valdi.modules

import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.InternedString
import java.lang.ref.WeakReference

interface ValdiBridgeObserverListener {
    fun bridgeObserverAddedNewCallback(observer: ValdiBridgeObserver)
}

class ValdiBridgeObserver: ValdiFunction {

    private var listener: ValdiBridgeObserverListener? = null

    private val callbacks = mutableListOf<ValdiFunction>()

    private fun appendCallback(callback: ValdiFunction) {
        synchronized(callbacks) {
            callbacks.add(callback)
            this.listener?.bridgeObserverAddedNewCallback(this)
        }
    }

    private fun removeCallback(callback: ValdiFunction) {
        synchronized(callbacks) {
            callbacks.remove(callback)
        }
    }

    fun setListener(listener: ValdiBridgeObserverListener?) {
        synchronized(callbacks) {
            this.listener = listener
        }
    }

    fun notifyObservers(marshaller: ValdiMarshaller) {
        val callbacksCopy = synchronized(callbacks) {
            callbacks.toTypedArray()
        }

        for (callback in callbacksCopy) {
            if (callback.perform(marshaller)) {
                marshaller.pop()
            }
        }
    }

    override fun perform(marshaller: ValdiMarshaller): Boolean {
        val callback = marshaller.getFunction(0)
        appendCallback(callback)

        val index = marshaller.pushMap(1)
        val weakCallback = WeakReference(callback)
        marshaller.putMapPropertyFunction(cancelString, index, object: ValdiFunction {
            override fun perform(marshaller: ValdiMarshaller): Boolean {
                val callback = weakCallback.get()
                if (callback != null) {
                    removeCallback(callback)
                }
                return false
            }
        })

        return true
    }

    companion object {
        val cancelString = InternedString.create("cancel")
    }

}
