package com.snap.valdi.utils

interface LoadCompletion<Item> {

    fun onSuccess(item: Item)

    fun onFailure(error: Throwable)

}