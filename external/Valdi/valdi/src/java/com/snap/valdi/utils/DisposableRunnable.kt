package com.snap.valdi.utils

/**
 * A Runnable implementation that can be disposed, which prevents the inner runnable to run.
 */
class DisposableRunnable(runnable: Runnable): DisposableCallback<Runnable>(runnable), Runnable {

    override fun run() {
        extractCallback()?.run()
    }

}