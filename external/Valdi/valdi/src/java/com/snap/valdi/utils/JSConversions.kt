package com.snap.valdi.utils

import com.snap.valdi.exceptions.AttributeError

// Convenience methods to extract values from a weakly typed dictionary.
// For now, parameters provided by JavaScript are weakly typed, so this can
// be used to have automatic conversions/casting to the expected type. In the future
// we might be able to remove all of this to have a strongly typed API.
object JSConversions {

    fun getParameterOrNull(parameters: Array<Any?>, index: Int): Any? {
        if (index >= parameters.size) {
            return null
        }
        return parameters[index]
    }

    fun asFloat(any: Any?): Float {
        if (any is Number) {
            return any.toFloat()
        } else if (any is String) {
            if (any.lastOrNull()?.isDigit() == false) {
                return any.trimEnd { !it.isDigit() }.toFloat()
            }
            return any.toFloat()
        }

        throw AttributeError("value $any is not a float")
    }

    fun asString(any: Any?): String {
        if (any is String) {
            return any
        } else if (any is Number) {
            return any.toString()
        }

        throw AttributeError("value $any is not a string")
    }

    fun asBoolean(any: Any?): Boolean {
        if (any is Boolean) {
            return any
        } else if (any is Number) {
            return any.toInt() != 0
        } else if (any is String) {
            return any.toBoolean()
        }

        throw AttributeError("value $any is not a boolean")
    }

    fun asInt(any: Any?): Int {
        if (any is Number) {
            return any.toInt()
        } else if (any is String) {
            return any.toInt()
        }

        throw AttributeError("value $any is not an int")
    }

    fun asLong(any: Any?): Long {
        if (any is Number) {
            return any.toLong()
        } else if (any is String) {
            return any.toLong()
        }

        throw AttributeError("value $any is not a long")
    }

    fun asDouble(any: Any?): Double {
        if (any is Number) {
            return any.toDouble()
        } else if (any is String) {
            return any.toDouble()
        }

        throw AttributeError("value $any is not a double")
    }

    private class WrappedJavaInstance(val instance: Any) {

        override fun toString(): String {
            return "WrappedJavaInstance: ${instance}"
        }

    }

    fun wrapNativeInstance(instance: Any): Any {
        return WrappedJavaInstance(instance)
    }

    fun unwrapNativeInstance(jsInstance: Any?): Any {
        if (jsInstance !is WrappedJavaInstance) {
            throw AttributeError("value $jsInstance is not a wrapped Java instance")
        }
        return jsInstance.instance
    }
}
