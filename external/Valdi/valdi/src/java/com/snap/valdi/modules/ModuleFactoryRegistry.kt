package com.snap.valdi.modules

import androidx.annotation.Keep
import com.snapchat.client.valdi_core.ModuleFactory
import com.snapchat.client.valdi_core.ModuleFactoriesProvider

import android.content.Context

@Keep
object ModuleFactoryRegistry {

    /**
    Register a module factory into the registry, which will make it
    available for newly created Valdi runtimes.
     */
    fun registerModuleFactory(cb: () -> ModuleFactory) {
        nativeRegister(object: ModuleFactoriesProvider() {
            override fun createModuleFactories(runtimeManager: Any): ArrayList<ModuleFactory> {
                return arrayListOf(cb())
            }
        })
    }

    fun registerModuleFactoryForClass(cls: Class<*>) {
        registerModuleFactory({
            val ctor = cls.getDeclaredConstructor()
            ctor.newInstance() as ModuleFactory
        })
    }

    fun registerModuleFactoriesFromContext(context: Context) {
        val assetManager = context.assets

        val nativeModulesList = assetManager.list("valdi_native_modules") ?: return

        for (clsName in nativeModulesList) {
            registerModuleFactoryForClass(Class.forName(clsName))
        }
    }

    @JvmStatic
    private external fun nativeRegister(obj: Any)
}
