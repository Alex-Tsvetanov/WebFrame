package com.snap.valdi.support

import android.app.Activity
import android.app.Application
import android.content.Context
import android.os.Bundle
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.Disposable
import com.snap.valdi.modules.ValdiBridgeModule
import com.snap.valdi.views.IScrollPerfLoggerBridge
import android.util.Log

private const val TAG = "ScrollPerfLoggerBridge";

class ScrollPerfLoggerBridge(val tag: String, val feature: String, val project: String): IScrollPerfLoggerBridge {
    override fun resume() {
        Log.d(TAG, "ScrollPerfLoggerBridge.resume $tag, $feature, $project resume()")
    }

    override fun pause(cancelLogging: Boolean) {
        Log.d(TAG, "ScrollPerfLoggerBridge $tag, $feature, $project pause($cancelLogging)")
    }
}

class ScrollBridgeLoggerFactoryModule(context: Context): ValdiBridgeModule() {

    override fun getModulePath(): String {
        return "GlobalScrollPerfLoggerBridgeFactory"
    }

    override fun loadModule(): Any {
        return mapOf(
                "createScrollPerfLoggerBridge" to makeBridgeMethod(this::createScrollPerfLoggerBridge)
        )
    }

    private fun createScrollPerfLoggerBridge(marshaller: ValdiMarshaller) {
        val tag = marshaller.getString(0)
        val feature = marshaller.getString(1)
        val project = marshaller.getString(2)

        val bridge = ScrollPerfLoggerBridge(tag, feature, project)
        Log.d(TAG, "Created a ScrollPerfLoggerBridge: $bridge")
        bridge.pushToMarshaller(marshaller)
    }

}
