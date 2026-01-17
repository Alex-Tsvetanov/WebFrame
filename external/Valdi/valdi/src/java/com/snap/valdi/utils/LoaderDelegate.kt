package com.snap.valdi.utils

interface LoaderDelegate<Key, Item> {

    fun load(key: Key, completion: LoadCompletion<Item>)

}