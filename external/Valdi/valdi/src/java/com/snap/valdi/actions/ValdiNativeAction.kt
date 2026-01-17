package com.snap.valdi.actions

import androidx.annotation.AnyThread
import androidx.annotation.UiThread
import com.snap.valdi.logger.LogLevel
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.runOnMainThreadIfNeeded
import java.lang.reflect.Method

/**
 * An implementation of ValdiAction which calls the given function name on the actionHandler.
 * It uses Java's reflection to call the method.
 * IMPORTANT: Note that you must make sure the method doesn't get mangled by Proguard.
 */
class ValdiNativeAction(private val actionHandlerHolder: ValdiActionHandlerHolder, private val functionName: String, private val logger: Logger): ValdiAction {

    private var lastActionHandlerClass: Class<*>? = null
    private var lastResolvedMethod: Method? = null
    private var lastResolvedMethodHasMapParam = false

    @UiThread
    private fun doPerform(parameters: Array<Any?>) {
        val actionHandler = actionHandlerHolder.actionHandler
        val actionHandlerClass = actionHandler?.javaClass
        if (actionHandlerClass != lastActionHandlerClass) {
            lastActionHandlerClass = actionHandlerClass
            lastResolvedMethod = null
        }
        if (actionHandler == null || actionHandlerClass == null) {
            return
        }

        if (lastResolvedMethod == null) {
            // Resolving the method to call on the actionHandler
            try {
                lastResolvedMethod = actionHandlerClass.getDeclaredMethod(functionName, Array<Any?>::class.java)!!
                lastResolvedMethodHasMapParam = true
            } catch (exc: NoSuchMethodException) {
                try {
                    lastResolvedMethod = actionHandlerClass.getDeclaredMethod(functionName)
                    lastResolvedMethodHasMapParam = false
                } catch (exc: NoSuchMethodException) {
                }
            }
        }

        val resolvedMethod = lastResolvedMethod

        if (resolvedMethod == null) {
            logger.log(LogLevel.ERROR, "Unable to call function $functionName on ${actionHandler::class.java}. ActionHandler does not implement method.")
            return
        }
        if (lastResolvedMethodHasMapParam) {
            resolvedMethod.invoke(actionHandler, parameters)
        } else {
            resolvedMethod.invoke(actionHandler)
        }
    }

    @AnyThread
    override fun perform(parameters: Array<Any?>): Any? {
        runOnMainThreadIfNeeded {
            doPerform(parameters)
        }

        return null
    }

}
