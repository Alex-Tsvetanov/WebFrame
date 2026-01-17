package com.snap.valdi.utils

/**
 * A LoadCompletion which can be disposed. When disposed, the delegated completion won't get called.
 */
class DisposableLoadCompletion<Item>(innerCompletion: LoadCompletion<Item>?):
        DisposableCallback<LoadCompletion<Item>>(innerCompletion),
        LoadCompletion<Item> {

    override fun onSuccess(item: Item) {
        extractCallback()?.onSuccess(item)
    }

    override fun onFailure(error: Throwable) {
        extractCallback()?.onFailure(error)
    }

}