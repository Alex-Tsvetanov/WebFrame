package com.snap.valdi.views.touches

import com.snap.valdi.callable.ValdiFunction

/**
 * Grouping of gesture recognizers.
 */
class GestureRecognizers {

    val gestureRecognizers = mutableListOf<ValdiGestureRecognizer>()
    var hitTest: ValdiFunction? = null

    fun isEmpty(): Boolean {
        return gestureRecognizers.isEmpty()
    }

    fun <T> getGestureRecognizer(gestureRecognizerClass: Class<T>): T? {
        val index = indexOfGestureRecognizer(gestureRecognizerClass)
        if (index >= 0) {
            @Suppress("UNCHECKED_CAST")
            return gestureRecognizers[index] as T
        }

        return null
    }

    fun removeAllGestureRecognizers() {
        while (!isEmpty()) {
            removeGestureRecognizerAt(gestureRecognizers.size - 1)
        }
    }

    private fun removeGestureRecognizerAt(index: Int) {
        gestureRecognizers.removeAt(index).destroy()
    }

    fun addGestureRecognizer(gestureRecognizer: ValdiGestureRecognizer) {
        removeGestureRecognizer(gestureRecognizer::class.java)
        gestureRecognizers.add(gestureRecognizer)
    }

    fun hasGestureRecognizer(gestureRecognizerClass: Class<*>): Boolean {
        return indexOfGestureRecognizer(gestureRecognizerClass) >= 0
    }

    fun removeGestureRecognizer(gestureRecognizerClass: Class<*>) {
        val index = indexOfGestureRecognizer(gestureRecognizerClass)
        if (index >= 0) {
            removeGestureRecognizerAt(index)
        }
    }

    private fun indexOfGestureRecognizer(gestureRecognizerClass: Class<*>): Int {
        return gestureRecognizers.indexOfFirst { it::class.java == gestureRecognizerClass }
    }
}
