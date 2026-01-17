package com.snap.valdi.utils

import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.schema.ValdiValueMarshallerRegistry

open class ValdiMarshallableObject() : ValdiMarshallable, DisposablePrivate {

    override fun __dispose__() {
        ValdiValueMarshallerRegistry.shared.disposeObject(this::class.java, this)
    }

    override fun pushToMarshaller(marshaller: ValdiMarshaller): Int {
        return marshall(this::class.java, marshaller, this)
    }

    override fun equals(other: Any?): Boolean {
        if (other == null) {
            return false
        }
        val cls = this::class.java
        val otherCls = other::class.java
        if (cls != otherCls) {
            return false
        }

        return ValdiValueMarshallerRegistry.shared.objectEquals(cls, this, other)
    }

    override fun toString(): String {
        return ValdiMarshaller.use {
            val objectIndex = pushToMarshaller(it)
            it.toString(objectIndex, true)
        }
    }

    fun toMap(): Map<String, Any?> {
        return marshallAsMap(this::class.java, this)
    }

    companion object {
        @JvmStatic
        fun <T> unmarshall(cls: Class<T>, marshaller: ValdiMarshaller, objectIndex: Int): T {
            return ValdiValueMarshallerRegistry.shared.unmarshallObject(
                    cls,
                    marshaller,
                    objectIndex)
        }

        @JvmStatic
        fun <T> fromUntyped(cls: Class<T>, untyped: Any?): T {
            return ValdiMarshaller.use {
                val objectIndex = it.pushUntyped(untyped)
                unmarshall(cls, it, objectIndex)
            }
        }

        @JvmStatic
        fun <T> fromMap(cls: Class<T>, map: Map<String, Any?>): T {
            return fromUntyped(cls, map)
        }

        @JvmStatic
        fun marshall(cls: Class<*>, marshaller: ValdiMarshaller, obj: Any): Int {
            return ValdiValueMarshallerRegistry.shared.marshallObject(
                    cls,
                    marshaller,
                    obj)
        }

        @JvmStatic
        fun marshallAsMap(cls: Class<*>, obj: Any): Map<String, Any?> {
            return ValdiValueMarshallerRegistry.shared.marshallObjectAsMap(
                    cls,
                    obj)
        }

        @JvmStatic
        fun unimplementedMethod(): Nothing {
            throw ValdiException("Unimplemented method")
        }
    }
}

/**
 * Make a shallow copy of the ValdiMarshallableObject.
 */
fun <T: ValdiMarshallableObject> T.copy(): T {
    val cls = this::class.java
    return ValdiValueMarshallerRegistry.shared.copyObject(cls, this)
}

/**
 * Copy the ValdiMarshallableObject and apply the given visitor on the copied object
 * to apply further modifications.
 */
inline fun <T: ValdiMarshallableObject> T.copying(visitor: T.() -> Unit): T {
    return copy().apply(visitor)
}
