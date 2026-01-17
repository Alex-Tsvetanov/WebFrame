package com.snap.valdi.support

import com.snap.valdi.IValdiRuntime
import com.snap.valdi.context.ValdiContext
import com.snap.valdi.navigation.INavigator
import com.snap.valdi.navigation.INavigatorPageConfig
import com.snap.valdi.utils.Disposable
import com.snap.valdi.utils.runOnMainThreadIfNeeded
import com.snap.valdi.views.ValdiRootView
import com.snap.valdi.views.ValdiView
import com.snap.valdi.utils.ValdiMarshaller

class DefaultNavigator(val navigationView: NavigationView, val runtime: IValdiRuntime): INavigator {

    override fun pushComponent(page: INavigatorPageConfig, animated: Boolean) {
        val sourceContext = ValdiContext.current()
        runOnMainThreadIfNeeded {
            val rootView = ValdiRootView(runtime.context)
            if (sourceContext != null) {
                rootView.getValdiContext {
                    it.setParentContext(sourceContext)
                }
            }

            rootView.setRetainsLayoutSpecsOnInvalidateLayout(true)

            val componentContext = page.componentContext.toMutableMap()
            componentContext["navigator"] = DefaultNavigator(navigationView, runtime)
            runtime.inflateViewAsync(rootView, page.componentPath, page.componentViewModel, componentContext, null, null)
            navigationView.push(rootView, animated)
        }
    }

    override fun pop(animated: Boolean) {
        runOnMainThreadIfNeeded {
            navigationView.pop(animated) {
                (it as? Disposable)?.dispose()
            }
        }
    }

    override fun popToRoot(animated: Boolean) {
        runOnMainThreadIfNeeded {
            var didPop = true
            while (didPop) {
                didPop = navigationView.pop(animated) {
                    (it as? Disposable)?.dispose()
                }
            }
        }
    }

    override fun popToSelf(animated: Boolean) {
        runOnMainThreadIfNeeded {

        }
    }

    override fun presentComponent(page: INavigatorPageConfig, animated: Boolean) {
        pushComponent(page, animated)
    }

    override fun dismiss(animated: Boolean) {
        pop(animated)
    }

    override fun forceDisableDismissalGesture(forceDisable: Boolean) {
    }

    override fun setBackButtonObserver(observer: (() -> Unit)?) {
    }

    override fun setOnPausePopAfterDelay(delayMs: Double?): Unit {
    }

}
