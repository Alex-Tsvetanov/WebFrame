package com.snap.valdi.actions

/**
 * Container objects for the actions in a Valdi document.
 */
class ValdiActions(val actionHandlerHolder: ValdiActionHandlerHolder, val actionByName: MutableMap<String, ValdiAction> = hashMapOf()) {

    fun register(name: String, action: ValdiAction) {
        actionByName[name] = action
    }

    fun register(name: String, runnable: (Array<Any>) -> Any?) {
        register(name, ValdiRunnableAction(runnable))
    }

    fun getAction(name: String): ValdiAction? {
        return actionByName[name]
    }
}
