package com.snap.valdi.modules

import com.snapchat.client.valdi_core.ModuleFactory

abstract class ValdiGeneratedModuleFactory<T: Any>: ModuleFactory() {

    abstract fun onLoadModule(): T

    final override fun loadModule(): Any {
        return onLoadModule()
    }
}