package com.snap.valdi.actions

/**
 * An implementation of ValdiAction backed by a given closure.
 */
class ValdiRunnableAction(val closure: (Array<Any>) -> Any?): ValdiAction {

    override fun perform(parameters: Array<Any>): Any? {
        return closure(parameters)
    }
}
