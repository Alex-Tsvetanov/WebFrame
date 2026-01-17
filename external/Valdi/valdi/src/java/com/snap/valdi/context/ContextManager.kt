package com.snap.valdi.context

import androidx.annotation.Keep
import com.snap.valdi.ValdiRuntime
import com.snapchat.client.valdi.NativeBridge
import com.snap.valdi.exceptions.ValdiFatalException
import com.snap.valdi.logger.Logger
import com.snap.valdi.nativebridge.ContextNative
import com.snap.valdi.utils.ValdiLeakTracker
import com.snapchat.client.valdi.utils.CppObjectWrapper
import java.lang.ref.WeakReference

/**
 * ContextManager is a private class used by native code to manage JVM's ValdiContexts.
 * Methods in this class are used by native code. Don't remove them!
 */
class ContextManager(private val nativeBridge: NativeBridge, private val logger: Logger) {

    @SuppressWarnings("unused")
    @Keep
    fun createContext(nativeHandle: Long, contextId: Int, componentPath: String, moduleName: String, owner: String,  runtime: Object): ValdiContext {
        if (nativeHandle == 0L) {
            ValdiFatalException.handleFatal("Unexpectedly got nullptr for native ValdiContext")
        }

        runtime as ValdiRuntime
        val contextNative = ContextNative(nativeHandle, nativeBridge, runtime.native)

        val context = ValdiContext(contextNative, contextId as ValdiContextId, componentPath, moduleName, owner, logger)

        // Uncomment to track context leaks
//        ValdiLeakTracker.trackRef(WeakReference(context), "ValdiContext ${context.componentPath}", runtime)
//        context.setDisableViewReuse(true)

        return context
    }

    @SuppressWarnings("unused")
    @Keep
    fun destroyContext(context: ValdiContext) {
        context.onDestroy()
    }

    @SuppressWarnings("unused")
    @Keep
    fun onContextRendered(context: ValdiContext) {
        context.onRender()

        val owner = context.owner ?: return
        val rootView = context.rootView ?: return

        owner.onRendered(rootView)
    }

    @SuppressWarnings("unused")
    @Keep
    fun onContextLayoutBecameDirty(context: ValdiContext) {
        context.onLayoutDidBecomeDirty()
    }

    @SuppressWarnings("unused")
    @Keep
    fun onAllContextsDestroyed(runtime: Any?) {

    }
}
