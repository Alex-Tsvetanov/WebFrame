package com.snap.valdi.preload

import com.snap.valdi.ValdiRuntimeManager
import com.snap.valdi.IValdiRuntime
import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.schema.ValdiValueMarshallerRegistry
import com.snap.valdi.utils.ValdiMarshallable
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.views.ValdiGeneratedRootView

/**
 * The Preloader provides some APIs to preload modules within the Valdi Runtime
 * so that they can evaluate faster later on.
 */
class ValdiPreloader(private val runtime: IValdiRuntime) {

    /**
    Preload the module given as an absolute path (e.g. 'valdi_core/src/Renderer').
    When maxDepth is more than 1, the preload will apply recursively to modules that the given
    modulePath imports, up until the given depth.
     */
    fun preloadModule(modulePath: String, maxDepth: Int) {
        runtime.getJSRuntime { it.preloadModule(modulePath, maxDepth) }
    }

    /**
    Preload the module given as a component path (e.g. 'MyClass@my_module/src/MyClass').
    When maxDepth is more than 1, the preload will apply recursively to modules that the given
    modulePath imports, up until the given depth.
     */
    fun preloadModuleFromComponentPath(componentPath: String) {
        val separator = componentPath.indexOf('@')
        if (separator < 0) {
            throw ValdiException("'${componentPath}' is not a componentPath")
        }

        preloadModule(componentPath.substring(separator + 1), 1)
    }

    /**
     * Preload the ValueMarshaller for the given class in a worker thread.
     */
    fun <T> preloadMarshaller(cls: Class<T>) {
        if (!ValdiMarshallable::class.java.isAssignableFrom(cls)) {
            return
        }

        runtime.getManager {
            it.enqueueWorkerTask(Runnable {
                ValdiMarshaller.use { marshaller ->
                    // This will force load the Java ValueMarshaller
                    ValdiValueMarshallerRegistry.shared.setActiveSchemaOfClassToMarshaller(cls, marshaller)
                }
            })
        }
    }

    companion object {
        /**
         * Preload the given root view class into the given runtime.
         * The JS modules that this module is using will be preloaded with a depth of 1,
         * meaning the imports that the root component is making will be eagerly loaded but not
         * the recursive imports from it. The value marshaller for the view model and component context
         * will be prepared.
         */
        @JvmStatic
        inline fun <T: ValdiGeneratedRootView<ViewModelType, ComponentContextType>,
                reified ViewModelType,
                reified ComponentContextType>
                preloadRootView(runtime: IValdiRuntime, componentPath: String, rootViewClass: Class<T>) {
            val preloader = ValdiPreloader(runtime)
            preloader.preloadModuleFromComponentPath(componentPath)
            preloader.preloadMarshaller(ViewModelType::class.java)
            preloader.preloadMarshaller(ComponentContextType::class.java)
        }

        /**
         * Preload the given root view class into all the main runtimes that are currently live.
         * The JS modules that this module is using will be preloaded with a depth of 1,
         * meaning the imports that the root component is making will be eagerly loaded but not
         * the recursive imports from it. The value marshaller for the view model and component context
         * will be prepared.
         */
        @JvmStatic
        inline fun <T: ValdiGeneratedRootView<ViewModelType, ComponentContextType>,
                reified ViewModelType,
                reified ComponentContextType>
                preloadRootView(componentPath: String, rootViewClass: Class<T>) {
            ValdiRuntimeManager.allRuntimes().forEach {
                if (it.isMain) {
                    preloadRootView(it, componentPath, rootViewClass)
                }
            }
        }
    }
}
