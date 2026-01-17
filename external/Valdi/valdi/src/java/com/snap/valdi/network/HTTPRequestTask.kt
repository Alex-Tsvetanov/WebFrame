package com.snap.valdi.network

import com.snapchat.client.valdi_core.Cancelable
import com.snapchat.client.valdi_core.HTTPRequestManagerCompletion
import com.snapchat.client.valdi_core.HTTPResponse

open class HTTPRequestTask(private var completion: HTTPRequestManagerCompletion?): Cancelable() {

    override fun cancel() {
        removeCompletion()
    }

    private fun removeCompletion(): HTTPRequestManagerCompletion? {
        return synchronized(this) {
            val completion = this.completion
            this.completion = null
            completion
        }
    }

    fun notifySuccess(response: HTTPResponse) {
        removeCompletion()?.onComplete(response)
    }

    fun notifyFailure(error: String) {
        removeCompletion()?.onFail(error)
    }

}