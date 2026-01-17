package com.snap.valdi.promise

fun <I, O> Promise<I>.then(cb: (input: I) -> O): Promise<O> {
    val output = ResolvablePromise<O>()
    this.onComplete(object: PromiseCallback<I> {
        override fun onSuccess(value: I) {
            val result = try {
                cb(value)
            } catch (exc: Throwable) {
                output.fulfillFailure(exc)
                return
            }

            output.fulfillSuccess(result)
        }

        override fun onFailure(error: Throwable) {
            output.fulfillFailure(error)
        }
    })

    return output
}