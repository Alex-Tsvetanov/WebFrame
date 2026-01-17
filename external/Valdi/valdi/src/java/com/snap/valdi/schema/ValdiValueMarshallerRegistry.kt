package com.snap.valdi.schema

import com.snap.valdi.utils.ValdiJNI
import com.snap.valdi.utils.ValdiMarshaller

typealias ValdiSchemaIdentifier = Int
interface ValdiValueMarshallerRegistry {

    fun marshallObject(cls: Class<*>, marshaller: ValdiMarshaller, obj: Any): Int
    fun marshallObjectAsMap(cls: Class<*>, obj: Any): Map<String, Any?>
    fun <T> unmarshallObject(cls: Class<T>, marshaller: ValdiMarshaller, objectIndex: Int): T
    fun <T> setActiveSchemaOfClassToMarshaller(cls: Class<T>, marshaller: ValdiMarshaller)
    fun getEnumStringValue(cls: Class<*>, value: Enum<*>): String
    fun getEnumIntValue(cls: Class<*>, value: Enum<*>): Int
    fun objectEquals(cls: Class<*>, left: Any, right: Any): Boolean

    fun <T: Any> copyObject(cls: Class<*>, obj: T): T

    fun disposeObject(cls: Class<*>, obj: Any)


    companion object {
        @JvmStatic
        val shared: ValdiValueMarshallerRegistry = if (ValdiJNI.available) {
            ValdiValueMarshallerRegistryCpp()
        } else {
            ValdiValueMarshallerRegistryJava()
        }
    }

}
