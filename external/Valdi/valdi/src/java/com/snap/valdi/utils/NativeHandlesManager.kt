package com.snap.valdi.utils

import com.snapchat.client.valdi.NativeBridge
import com.snapchat.client.valdi.utils.CppObjectWrapper
import java.lang.ref.PhantomReference
import java.lang.ref.ReferenceQueue

object NativeHandlesManager {

    private class CppObjectReference(referent: CppObjectWrapper, q: ReferenceQueue<CppObjectWrapper>) : PhantomReference<CppObjectWrapper>(referent, q) {

        private val nativeHandle = referent.nativeHandle

        fun destroy() {
            NativeBridge.deleteNativeHandle(nativeHandle)
        }
    }

    private val referenceQueue = ReferenceQueue<CppObjectWrapper>()
    private val references = hashSetOf<CppObjectReference>()
    private var thread: Thread? = null

    private val started: Boolean
        get() = thread != null

    fun add(handle: CppObjectWrapper): Boolean {
        if (!started) {
            return false
        }

        synchronized(references) {
            references.add(CppObjectReference(handle, this.referenceQueue))
        }

        return true
    }

    fun start() {
        if (thread != null) {
            return
        }

        this.thread = Thread(this::removeReferences, "Valdi Finalizer Thread").apply {
            priority = QoSClass.LOW.toThreadPriority()
        }
        this.thread?.start()
    }

    fun getReachableReferencesCount(): Int {
        return synchronized(references) {
            references.size
        }
    }

    private fun removeReferences() {
        while (true) {
            val reference = referenceQueue.remove() as CppObjectReference

            val removed = synchronized(references) {
                references.remove(reference)
            }

            if (removed) {
                reference.destroy()
            }
        }
    }

}