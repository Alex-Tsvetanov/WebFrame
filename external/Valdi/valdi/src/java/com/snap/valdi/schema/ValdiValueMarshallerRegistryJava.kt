package com.snap.valdi.schema

import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.utils.ValdiMarshallableObject
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.arrayMap
import java.lang.reflect.ParameterizedType
import java.lang.reflect.Type
import java.lang.reflect.WildcardType

/**
 * An incomplete implementation of the ValdiValueMarshallerRegistry for use in pure Java
 * environments, when running unit tests for example.
 */
class ValdiValueMarshallerRegistryJava: ValdiValueMarshallerRegistry {

    private val classDelegateManager = ValdiClassDelegateManager()

    private fun marshallValue(marshaller: ValdiMarshaller, value: Any?): Int {
        return when (value) {
            is ValdiMarshallableObject -> {
                marshallObject(value::class.java, marshaller, value)
            }
            is List<*> -> {
                marshaller.pushList(value) {
                    marshallValue(marshaller, it)
                }
            }
            is Enum<*> -> {
                marshallObject(value::class.java, marshaller, value)
            }
            else -> {
                marshaller.pushUntyped(value)
            }
        }
    }

    private fun unmarshallValue(marshaller: ValdiMarshaller,
                                type: Type,
                                objectIndex: Int): Any? {
        return if (type is ParameterizedType) {
            val actualType = type.rawType as Class<*>
            if (List::class.java.isAssignableFrom(actualType)) {
                val itemType = type.actualTypeArguments.first()
                marshaller.getList(objectIndex) {
                    unmarshallValue(marshaller, itemType, it)
                }
            } else {
                marshaller.getUntyped(objectIndex)
            }
        } else if (type is Class<*>) {
            if (ValdiMarshallableObject::class.java.isAssignableFrom(type) || type.isEnum) {
                unmarshallObject(type, marshaller, objectIndex)
            } else if (List::class.java.isAssignableFrom(type)) {
                marshaller.getUntypedList(objectIndex)
            } else {
                marshaller.getUntyped(objectIndex)
            }
        } else if (type is WildcardType) {
            unmarshallValue(marshaller, type.upperBounds.first(), objectIndex)
        } else {
            marshaller.getUntyped(objectIndex)
        }
    }

    override fun marshallObject(cls: Class<*>, marshaller: ValdiMarshaller, obj: Any): Int {
        return when (val classDelegate = classDelegateManager.getClassDelegate(cls)) {
            is ValdiClassDelegateManager.ValdiObjectClassDelegate -> {
                val objetIndex = marshaller.pushMap(classDelegate.fieldDelegates.size)
                for (fieldDelegate in classDelegate.fieldDelegates) {
                    marshaller.putMapProperty(fieldDelegate.name, objetIndex) {
                        marshallValue(marshaller, fieldDelegate.field.get(obj))
                    }
                }

                objetIndex
            }
            is ValdiClassDelegateManager.ValdiInterfaceClassDelegate -> {
                marshaller.pushOpaqueObject(obj)
            }
            is ValdiClassDelegateManager.ValdiUntypedClassDelegate -> {
                marshaller.pushUntyped(obj)
            }
            is ValdiClassDelegateManager.ValdiEnumClassDelegate -> {
                marshaller.pushInt((obj as Enum<*>).ordinal)
            }
            else -> throw ValdiException("Unsupported ClassDelegate ${classDelegate::class.java}")
        }
    }

    override fun marshallObjectAsMap(cls: Class<*>, obj: Any): Map<String, Any?> {
        return ValdiMarshaller.use {
            val objectIndex = marshallObject(cls, it, obj)
            it.getUntypedCasted(objectIndex)
        }
    }

    @Suppress("UNCHECKED_CAST")
    override fun <T> unmarshallObject(cls: Class<T>, marshaller: ValdiMarshaller, objectIndex: Int): T {
        return when (val classDelegate = classDelegateManager.getClassDelegate(cls)) {
            is ValdiClassDelegateManager.ValdiObjectClassDelegate -> {
                classDelegate.newObject { fieldDelegate ->
                    marshaller.getMapPropertyOptional(fieldDelegate.name, objectIndex) {
                        unmarshallValue(marshaller, fieldDelegate.field.genericType, it)
                    }
                } as T
            }
            is ValdiClassDelegateManager.ValdiInterfaceClassDelegate -> {
                marshaller.getUntyped(objectIndex) as T
            }
            is ValdiClassDelegateManager.ValdiUntypedClassDelegate -> {
                marshaller.getUntyped(objectIndex) as T
            }
            is ValdiClassDelegateManager.ValdiEnumClassDelegate -> {
                classDelegate.enumValues[marshaller.getInt(objectIndex)] as T
            }
            else -> throw ValdiException("Unsupported ClassDelegate ${classDelegate::class.java}")
        }
    }

    override fun <T> setActiveSchemaOfClassToMarshaller(cls: Class<T>, marshaller: ValdiMarshaller) {
        // no-op
    }

    override fun getEnumIntValue(cls: Class<*>, value: Enum<*>): Int {
        return value.ordinal
    }

    override fun getEnumStringValue(cls: Class<*>, value: Enum<*>): String {
        return value.name
    }

    override fun <T: Any> copyObject(cls: Class<*>, obj: T): T {
        @Suppress("UNCHECKED_CAST")
        return classDelegateManager.getClassDelegate(cls).copyObject(obj) as T
    }

    override fun objectEquals(cls: Class<*>, left: Any, right: Any): Boolean {
        return classDelegateManager.getClassDelegate(cls).objectEquals(left, right)
    }

    override fun disposeObject(cls: Class<*>, obj: Any) {
        classDelegateManager.getClassDelegate(cls).disposeObject(obj)
    }

}
