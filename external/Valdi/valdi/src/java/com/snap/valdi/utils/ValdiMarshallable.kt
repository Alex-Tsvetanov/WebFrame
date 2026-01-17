package com.snap.valdi.utils

import androidx.annotation.Keep

/**
 * Object that can push a representation of itself into a Marshaller.
 */
@Keep
interface ValdiMarshallable {

    fun pushToMarshaller(marshaller: ValdiMarshaller): Int

    companion object {
        fun from(obj: Any?): ValdiMarshallable? {
            if (obj == null) {
                return null
            }

            if (obj is ValdiMarshallable) {
                return obj
            }

            return object: ValdiMarshallable {
                override fun pushToMarshaller(marshaller: ValdiMarshaller): Int {
                    return marshaller.pushUntyped(obj)
                }

            }
        }
    }
}

/**
 * Compares two ValdiMarshallable instances using a slow method which
 * involves marshalling the items to compare them.
 */
fun ValdiMarshallable.slowEqualsTo(other: Any?): Boolean {
    if (other !is ValdiMarshallable) {
        return false
    }
    if (this === other) {
        return true
    }

    return ValdiMarshaller.use { leftMarshaller ->
        pushToMarshaller(leftMarshaller)

        ValdiMarshaller.use { rightMarshaller ->
            other.pushToMarshaller(rightMarshaller)

            leftMarshaller == rightMarshaller
        }
    }

}

fun ValdiMarshallable.slowToString(indent: Boolean): String {
    return ValdiMarshaller.use {
        val itemIndex = pushToMarshaller(it)
        it.toString(itemIndex, indent)
    }
}
