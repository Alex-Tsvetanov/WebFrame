package com.snap.valdi.modules

import com.snap.valdi.IValdiRuntime
import com.snap.valdi.jsmodules.ValdiStringsModule
import com.snap.valdi.utils.Disposable
import com.snapchat.client.valdi_core.ModuleFactory

class ValdiNativeModules(
        val applicationModule: ValdiApplicationModule,
        val deviceModule: ValdiDeviceModule,
        val dateFormattingModule: ValdiDateFormattingModule,
        val numberFormattingModule: ValdiNumberFormattingModule,
        val drawingModule: DrawingModuleImpl,
        val stringsModule: ValdiStringsModule
): Disposable {

    override fun dispose() {
        applicationModule.dispose()
    }

    fun all(): Array<ModuleFactory> {
        return arrayOf(
                applicationModule,
                deviceModule,
                dateFormattingModule,
                numberFormattingModule,
                drawingModule,
                stringsModule)
    }

    fun register(runtime: IValdiRuntime) {
        all().forEach { runtime.registerNativeModuleFactory(it) }
    }
}
