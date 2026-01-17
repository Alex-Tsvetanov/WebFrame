package com.snap.valdi.utils

import com.snap.valdi.actions.ValdiAction

import com.snap.valdi.callable.ValdiFunctionNative
import com.snapchat.client.valdi.utils.ValdiCPPAction

fun ValdiAction.performSync(parameters: Array<Any?>): Any? {
    return if (this is ValdiFunctionNative) {
        this.perform(ValdiFunctionNative.FLAGS_CALL_SYNC, parameters)
    } else {
        this.perform(parameters)
    }
}

fun ValdiAction.dispose() {
    if (this is ValdiCPPAction) {
        this.destroy()
    }
}
