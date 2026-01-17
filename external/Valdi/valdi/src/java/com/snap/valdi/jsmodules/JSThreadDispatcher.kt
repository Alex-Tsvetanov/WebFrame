package com.snap.valdi.jsmodules

import java.lang.Runnable

interface JSThreadDispatcher {

    fun runOnJsThread(runnable: Runnable)

}