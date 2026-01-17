package com.snap.valdi.helloworld

import com.snapchat.client.valdi_core.ModuleFactory
import com.snap.valdi.modules.RegisterValdiModule
import com.snap.valdi.modules.hello_world.NativeModuleModuleFactory
import com.snap.valdi.modules.hello_world.NativeModuleModule

@RegisterValdiModule
class MyNativeModuleFactory: NativeModuleModuleFactory() {

    override fun onLoadModule(): NativeModuleModule {
        return object: NativeModuleModule {
            override val APP_NAME = "Valdi Android"
        }
    }
}