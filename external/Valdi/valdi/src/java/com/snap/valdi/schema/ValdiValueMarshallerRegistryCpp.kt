package com.snap.valdi.schema

import androidx.annotation.Keep
import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.utils.ValdiMarshaller
import com.snapchat.client.valdi.utils.NativeHandleWrapper

@Keep
class ValdiValueMarshallerRegistryCpp: NativeHandleWrapper(nativeCreate()), ValdiValueMarshallerRegistry {

    private val classDelegateManager = ValdiClassDelegateManager()

    override fun destroyHandle(handle: Long) {
        nativeDestroy(handle)
    }

    override fun marshallObject(cls: Class<*>, marshaller: ValdiMarshaller, obj: Any): Int {
        return nativeMarshallObject(nativeHandle, cls.name, marshaller.nativeHandle, obj)
    }

    override fun marshallObjectAsMap(cls: Class<*>, obj: Any): Map<String, Any?> {
        @Suppress("UNCHECKED_CAST")
        return nativeMarshallObjectAsMap(nativeHandle, cls.name, obj) as Map<String, Any?>
    }

    override fun <T> unmarshallObject(cls: Class<T>, marshaller: ValdiMarshaller, objectIndex: Int): T {
        @Suppress("UNCHECKED_CAST")
        return nativeUnmarshallObject(nativeHandle, cls.name, marshaller.nativeHandle, objectIndex) as T
    }

    override fun <T> setActiveSchemaOfClassToMarshaller(cls: Class<T>, marshaller: ValdiMarshaller) {
        nativeSetActiveSchema(nativeHandle, cls.name, marshaller.nativeHandle)
    }

    override fun getEnumStringValue(cls: Class<*>, value: Enum<*>): String {
        val outValue = nativeGetEnumValue(nativeHandle, cls.name, value)
        if (outValue is String) {
            return outValue
        }
        throw ValdiException("Enum ${cls.name} is not a string enum")
    }

    override fun getEnumIntValue(cls: Class<*>, value: Enum<*>): Int {
        val outValue = nativeGetEnumValue(nativeHandle, cls.name, value)
        if (outValue is Int) {
            return outValue
        }
        throw ValdiException("Enum ${cls.name} is not a int enum")
    }

    override fun objectEquals(cls: Class<*>, left: Any, right: Any): Boolean {
        return classDelegateManager.getClassDelegate(cls).objectEquals(left, right)
    }

    override fun <T: Any> copyObject(cls: Class<*>, obj: T): T {
        @Suppress("UNCHECKED_CAST")
        return classDelegateManager.getClassDelegate(cls).copyObject(obj) as T
    }

    override fun disposeObject(cls: Class<*>, obj: Any) {
        classDelegateManager.getClassDelegate(cls).disposeObject(obj)
   }

    companion object {
        @JvmStatic
        private external fun nativeCreate(): Long
        @JvmStatic
        private external fun nativeDestroy(ptr: Long)
        @JvmStatic
        private external fun nativeMarshallObject(ptr: Long,
                                                  className: String,
                                                  marshallerHandle: Long,
                                                  obj: Any): Int

        @JvmStatic
        private external fun nativeMarshallObjectAsMap(ptr: Long,
                                                       className: String,
                                                       obj: Any): Any

        @JvmStatic
        private external fun nativeUnmarshallObject(ptr: Long,
                                                    className: String,
                                                    marshallerHandle: Long,
                                                    objectIndex: Int): Any?

        @JvmStatic
        private external fun nativeSetActiveSchema(ptr: Long,
                                                   className: String,
                                                   marshallerHandle: Long)

        @JvmStatic
        private external fun nativeGetEnumValue(ptr: Long,
                                                className: String,
                                                value: Any): Any
    }

}
