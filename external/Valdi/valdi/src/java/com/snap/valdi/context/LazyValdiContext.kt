package com.snap.valdi.context

import com.snap.valdi.ValdiContextConfiguration
import com.snap.valdi.IValdiRuntime
import com.snap.valdi.modules.drawing.Size
import com.snap.valdi.nodes.IValdiViewNode
import com.snap.valdi.utils.Disposable
import com.snap.valdi.utils.IAsyncWorkScheduler
import com.snap.valdi.views.ValdiRootView
import java.util.concurrent.locks.Lock
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

class LazyValdiContext(private val runtime: IValdiRuntime,
                          private val scheduler: IAsyncWorkScheduler?,
                          private val componentPath: String,
                          viewModel: Any?,
                          componentContext: Any?,
                          private val configuration: ValdiContextConfiguration?): IValdiContext {

    private class PendingLayoutSpecs(val width: Int, val height: Int, val isRTL: Boolean)
    private class PendingViewport(val x: Int, val y: Int, val width: Int, val height: Int)

    private val lock: Lock = ReentrantLock()
    private var innerViewModel = viewModel
    private var componentContext = componentContext

    private var innerRootView: ValdiRootView? = null
    private var innerValdiContext: ValdiContext? = null
    private var pendingLayoutSpecs: PendingLayoutSpecs? = null
    private var pendingViewport: PendingViewport? = null
    private var pendingWaitUntilAllUpdatesCompleted: MutableList<() -> Unit>? = null
    private var pendingEnqueueNextRenderCallback: MutableList<() -> Unit>? = null
    private var pendingOnNextLayout: MutableList<() -> Unit>? = null
    private var pendingRetainsLayoutSpecsOnInvalidateLayout: Boolean? = null
    private var scheduledWorkDisposable: Disposable? = null
    private var destroyed = false
    private var viewModelChanged = false
    private var scheduledCreateValdiContext = false
    private var innerContextId = 0

    override val contextId: ValdiContextId
        get() = lock.withLock { innerContextId }

    override var rootView: ValdiRootView?
        get() = lock.withLock { innerRootView }
        set(value) {
            lock.withLock {
                if (destroyed) {
                    return
                }

                innerRootView = value

                val valdiContext = this.innerValdiContext
                if (valdiContext != null) {
                    valdiContext.rootView = value
                } else if (value != null) {
                    createValdiContextIfNeeded()
                } else if (pendingWaitUntilAllUpdatesCompleted == null && scheduledWorkDisposable != null) {
                    // Cancel the load if we kicked it off earlier from a previous rootView instance
                    // and that we didn't actually start the createValdiContext call
                    val disposable = this.scheduledWorkDisposable
                    this.scheduledWorkDisposable = null
                    disposable?.dispose()
                }
            }
        }

    override var viewModel: Any?
        get() = lock.withLock { innerViewModel }
        set(value) {
            lock.withLock {
                if (destroyed) {
                    return
                }

                viewModelChanged = true
                innerViewModel = value
                innerValdiContext?.viewModel = value
            }
        }

    private fun onValdiContextReady(valdiContext: ValdiContext) {
        lock.withLock {
            this.scheduledWorkDisposable = null

            val pendingWaitUntilAllUpdatesCompleted = this.pendingWaitUntilAllUpdatesCompleted
            this.pendingWaitUntilAllUpdatesCompleted = null

            val pendingEnqueueNextRenderCallback = this.pendingEnqueueNextRenderCallback
            val pendingOnNextLayout = this.pendingOnNextLayout
            this.pendingEnqueueNextRenderCallback = null
            this.pendingOnNextLayout = null

            if (destroyed) {
                valdiContext.destroy()
                pendingWaitUntilAllUpdatesCompleted?.forEach { it() }
                return
            }

            this.innerValdiContext = valdiContext
            this.innerContextId = valdiContext.contextId

            if (viewModelChanged) {
                viewModelChanged = false
                valdiContext.viewModel = innerViewModel
            }

            if (innerRootView != null) {
                valdiContext.rootView = innerRootView
            }

            val pendingLayoutSpecs = this.pendingLayoutSpecs
            val pendingViewport = this.pendingViewport
            val pendingRetainsLayoutSpecsOnInvalidateLayout = this.pendingRetainsLayoutSpecsOnInvalidateLayout
            this.pendingLayoutSpecs = null
            this.pendingViewport = null
            this.pendingRetainsLayoutSpecsOnInvalidateLayout = null

            if (pendingViewport != null) {
                valdiContext.setVisibleViewport(pendingViewport.x, pendingViewport.y, pendingViewport.width, pendingViewport.height)
            }

            if (pendingLayoutSpecs != null) {
                valdiContext.setLayoutSpecs(pendingLayoutSpecs.width, pendingLayoutSpecs.height, pendingLayoutSpecs.isRTL)
            }

            if (pendingRetainsLayoutSpecsOnInvalidateLayout != null) {
                valdiContext.setRetainsLayoutSpecsOnInvalidateLayout(pendingRetainsLayoutSpecsOnInvalidateLayout)
            }

            pendingWaitUntilAllUpdatesCompleted?.forEach { valdiContext.waitUntilAllUpdatesCompleted(it) }
            pendingEnqueueNextRenderCallback?.forEach{ valdiContext.enqueueNextRenderCallback(it) }
            pendingOnNextLayout?.forEach { valdiContext.onNextLayout(it) }
        }
    }

    private fun appendWaitUntilAllUpdatesCompleted(callback: () -> Unit) {
        if (pendingWaitUntilAllUpdatesCompleted == null) {
            pendingWaitUntilAllUpdatesCompleted = arrayListOf()
        }
        pendingWaitUntilAllUpdatesCompleted?.add(callback)
    }

    private fun doCreateValdiContext(completion: (() -> Unit)?) {
        scheduledCreateValdiContext = true
        scheduledWorkDisposable = null
        viewModelChanged = false

        if (completion != null) {
            appendWaitUntilAllUpdatesCompleted(completion)
        }

        runtime.createValdiContext(componentPath, innerViewModel, componentContext, null, configuration) {
            onValdiContextReady(it)
        }
    }

    private fun createValdiContextIfNeeded() {
        if (scheduledCreateValdiContext) {
            return
        }

        if (scheduler != null) {
            if (scheduledWorkDisposable != null) {
                return
            }

            scheduledWorkDisposable = scheduler.scheduleWork {
                lock.withLock {
                    doCreateValdiContext(it)
                }
            }
        } else {
            // lock already acquired by callsites calling `createValdiContextIfNeeded`
            doCreateValdiContext(null)
        }
    }

    override fun waitUntilAllUpdatesCompleted(callback: () -> Unit) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.waitUntilAllUpdatesCompleted(callback)
            } else {
                if (pendingWaitUntilAllUpdatesCompleted == null) {
                    pendingWaitUntilAllUpdatesCompleted = arrayListOf()
                }
                pendingWaitUntilAllUpdatesCompleted?.add(callback)
                createValdiContextIfNeeded()
            }
        }
    }

    override fun onNextLayout(callback: () -> Unit) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.onNextLayout(callback)
            } else {
                if (pendingOnNextLayout == null) {
                    pendingOnNextLayout = arrayListOf()
                }
                pendingOnNextLayout?.add(callback)
            }
        }
    }

    override fun measureLayout(widthMeasureSpec: Int, heightMeasureSpec: Int, isRTL: Boolean): Size {
        lock.withLock {
            val valdiContext = this.innerValdiContext ?: return Size(0.0, 0.0)
            return valdiContext.measureLayout(widthMeasureSpec, heightMeasureSpec, isRTL)
        }
    }

    override fun setLayoutSpecs(width: Int, height: Int, isRTL: Boolean) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.setLayoutSpecs(width, height, isRTL)
            } else {
                this.pendingLayoutSpecs = PendingLayoutSpecs(width, height, isRTL)
            }
        }
    }

    override fun setVisibleViewport(x: Int, y: Int, width: Int, height: Int) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.setVisibleViewport(x, y, width, height)
            } else {
                this.pendingViewport = PendingViewport(x, y, width, height)
            }
        }
    }

    override fun unsetVisibleViewport() {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.unsetVisibleViewport()
            } else {
                this.pendingViewport = null
            }
        }
    }

    override fun getViewNode(viewId: String): IValdiViewNode? {
        return lock.withLock { this.innerValdiContext?.getViewNode(viewId) }
    }

    override fun getViewNodeForId(viewNodeId: Int): IValdiViewNode? {
        return lock.withLock { this.innerValdiContext?.getViewNodeForId(viewNodeId) }
    }

    override fun getRootViewNode(): IValdiViewNode? {
        return lock.withLock { this.innerValdiContext?.getRootViewNode() }
    }

    override fun setRetainsLayoutSpecsOnInvalidateLayout(retainsLayoutSpecsOnInvalidateLayout: Boolean) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.setRetainsLayoutSpecsOnInvalidateLayout(retainsLayoutSpecsOnInvalidateLayout)
            } else {
                this.pendingRetainsLayoutSpecsOnInvalidateLayout = retainsLayoutSpecsOnInvalidateLayout
            }
        }
    }

    override fun scheduleExclusiveUpdate(callback: Runnable) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.scheduleExclusiveUpdate(callback)
            } else {
                callback.run()
            }
        }
    }

    override fun enqueueNextRenderCallback(callback: () -> Unit) {
        lock.withLock {
            val valdiContext = this.innerValdiContext
            if (valdiContext != null) {
                valdiContext.enqueueNextRenderCallback(callback)
            } else {
                if (pendingEnqueueNextRenderCallback == null) {
                    pendingEnqueueNextRenderCallback = arrayListOf()
                }
                pendingEnqueueNextRenderCallback?.add(callback)
            }
        }
    }

    override fun destroy() {
        val valdiContext = lock.withLock {
            destroyed = true
            this.innerRootView = null
            this.innerViewModel = null
            this.componentContext = null
            this.pendingLayoutSpecs = null
            this.pendingRetainsLayoutSpecsOnInvalidateLayout = null

            val valdiContext = this.innerValdiContext
            this.innerValdiContext = null
            valdiContext
        }

        valdiContext?.destroy()
    }

    override fun isDestroyed(): Boolean {
        lock.withLock {
            return destroyed
        }
    }
}
