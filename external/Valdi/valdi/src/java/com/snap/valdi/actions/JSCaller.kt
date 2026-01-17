package com.snap.valdi.actions

/**
 * Abstraction over a way to call a JS function by its name
 */
interface JSCaller {

    fun call(function: String, parameters: Array<Any?>)

}