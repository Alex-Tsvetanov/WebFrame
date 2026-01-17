package com.snap.valdi.support

import android.content.Context
import android.util.Log
import com.snap.valdi.ValdiTweaks
import com.snap.valdi.ValdiRuntimeManager
import com.snap.valdi.DebugMessagePresenter
import com.snap.valdi.RenderBackend
import com.snap.valdi.ExceptionReporter
import com.snap.valdi.utils.trace
import com.snap.valdi.modules.ModuleFactoryRegistry

object SupportValdiRuntimeManager {

    private const val TAG = "Valdi"

    @JvmStatic
    fun createWithSupportLibs(context: Context, tweaks: ValdiTweaks? = null): ValdiRuntimeManager {
        ModuleFactoryRegistry.registerModuleFactoriesFromContext(context)

        val resolvedTweaks = tweaks ?: ValdiTweaks()
        val runtimeManager = trace({ "Valdi.createRuntimeManager"}) {
            ValdiRuntimeManager(context.applicationContext, tweaks = resolvedTweaks)
        }

        runtimeManager.registerGlobalAttributesBindersFromContext(context)

        trace({ "Valdi.configureSupportLibs"}) {
            configureSupportLibs(runtimeManager)
        }

        return runtimeManager
    }

    @JvmStatic
    fun configureSupportLibs(runtimeManager: ValdiRuntimeManager) {
        if (runtimeManager.debugMessagePresenter == null) {
            runtimeManager.debugMessagePresenter = object: DebugMessagePresenter {
                override fun presentDebugMessage(level: DebugMessagePresenter.Level, str: String) {
                    Log.d(TAG, "Debug message: $str")
                }
            }
        }

        if (runtimeManager.exceptionReporter == null) {
            runtimeManager.exceptionReporter = object: ExceptionReporter {
                override fun reportNonFatal(errorCode: Int, message: String, module: String?, stackTrace: String?) {
                    val stackTraceString = stackTrace?.let { "\nStacktrace:\n$it" }
                    Log.e(TAG, "Uncaught error in module $module: $message $stackTraceString")
                }

                override fun reportCrash(message: String, module: String?, stackTrace: String?, isANR: Boolean) {
                    val stackTraceString = stackTrace?.let { "\nStacktrace:\n$it" }
                    if (isANR) {
                        Log.e(TAG, "ANR in module $module: $message $stackTraceString")
                    } else {
                        Log.e(TAG, "Crash in module $module: $message $stackTraceString")
                    }
                }
            }
        }

        runtimeManager.enqueueLoadOperation(Runnable {
            trace({ "Valdi.registerSupportFonts"}) {
                SupportFonts.registerFonts(runtimeManager)
            }
        })

        trace({ "Valdi.registerSupportModules"}) {
            SupportModules.registerSupportModules(runtimeManager)
        }
    }
}

