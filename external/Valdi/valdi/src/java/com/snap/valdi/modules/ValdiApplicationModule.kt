package com.snap.valdi.modules

import android.app.Activity
import android.app.Application
import android.content.Context
import android.os.Bundle
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.Disposable
import java.util.concurrent.atomic.AtomicBoolean

class ValdiApplicationModule(context: Context, isIntegrationTestEnvironment: Boolean): ValdiBridgeModule(), Application.ActivityLifecycleCallbacks, Disposable {

    private var enteredBackground =  ValdiBridgeObserver()
    private val enteredForeground = ValdiBridgeObserver()
    private val keyboardHeight = ValdiBridgeObserver()

    private var application: Application?

    private val displayScale: Double
    private val isAppInForeground = AtomicBoolean(false)
    private var isIntegrationTestEnvironment = false
    private val appVersion: String

    init {
        // Seems like most of time (always?) the applicationContext is the Application itself
        // There are other parts of the app that makes this assumption.
        this.application = context.applicationContext as? Application
        if (application != null) {
            application?.registerActivityLifecycleCallbacks(this)
        }

        val displayMetrics = context.resources.displayMetrics
        displayScale = displayMetrics.density.toDouble()
        this.isIntegrationTestEnvironment = isIntegrationTestEnvironment

        val packageInfo = context.packageManager.getPackageInfo(context.packageName, 0)
        appVersion = (packageInfo.versionName ?: "").split(" ").dropLastWhile(String::isEmpty).getOrNull(0)?.trim() ?: ""
    }

    override fun getModulePath(): String {
        return "valdi_core/src/ApplicationBridge"
    }

    override fun loadModule(): Any {
        return mapOf(
                "observeEnteredBackground" to enteredBackground,
                "observeEnteredForeground" to enteredForeground,
                "observeKeyboardHeight" to keyboardHeight,
                "isForegrounded" to makeBridgeMethod(this::isForegrounded),
                "isIntegrationTestEnvironment" to makeBridgeMethod(this::isIntegrationTestEnvironment),
                "getAppVersion" to makeBridgeMethod(this::getAppVersion)
        )
    }

    override fun onActivityPaused(activity: Activity) {
        isAppInForeground.set(false)
        ValdiMarshaller.use {
            enteredBackground.notifyObservers(it)
        }
    }

    override fun onActivityResumed(activity: Activity) {
        isAppInForeground.set(true)
        ValdiMarshaller.use {
            enteredForeground.notifyObservers(it)
        }
    }

    override fun onActivityStarted(activity: Activity) {
    }

    override fun onActivityDestroyed(activity: Activity) {
    }

    override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) {
    }

    override fun onActivityStopped(activity: Activity) {
    }

    override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) {
    }

    override fun dispose() {
        application?.unregisterActivityLifecycleCallbacks(this)
    }

    fun setIsAppInForeground(inForeground: Boolean) {
        isAppInForeground.set(inForeground)
    }

    fun notifyKeyboardHeightChange(heightInPx: Double) {
        val height = heightInPx / displayScale
        ValdiMarshaller.use {
            it.pushDouble(height)
            keyboardHeight.notifyObservers(it)
        }
    }

    private fun isForegrounded(marshaller: ValdiMarshaller) {
        marshaller.pushBoolean(isAppInForeground.get())
    }

    private fun getAppVersion(marshaller: ValdiMarshaller) {
        marshaller.pushString(appVersion)
    }

    private fun isIntegrationTestEnvironment(marshaller: ValdiMarshaller) {
        marshaller.pushBoolean(isIntegrationTestEnvironment)
    }
}
