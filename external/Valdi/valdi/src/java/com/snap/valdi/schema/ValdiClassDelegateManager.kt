package com.snap.valdi.schema

import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.utils.Disposable
import com.snap.valdi.utils.DisposableUtils
import com.snap.valdi.utils.arrayMap
import java.lang.reflect.Constructor
import java.lang.reflect.Field

/**
 * Manages ClassDelegate to allow Java to perform some operations on Marshallable objects
 * without JNI.
 */
class ValdiClassDelegateManager {

    abstract class ValdiFieldDelegate(val field: Field) {
        val name: String = field.name.run {
            if (startsWith("_")) {
                this.substring(1)
            } else {
                this
            }
        }

        abstract fun equals(left: Any, right: Any): Boolean

        fun getValue(obj: Any): Any? {
            return field.get(obj)
        }

        open fun disposeHeldValue(obj: Any): Boolean {
            return false
        }
    }

    interface ValdiClassDelegate {
        fun objectEquals(left: Any, right: Any): Boolean

        fun copyObject(obj: Any): Any

        fun disposeObject(obj: Any)
    }

    class ValdiObjectClassDelegate(val manager: ValdiClassDelegateManager,
                                      val constructor: Constructor<*>,
                                      val fieldDelegates: Array<ValdiFieldDelegate>): ValdiClassDelegate {
        override fun objectEquals(left: Any, right: Any): Boolean {
            for (fieldDelegate in fieldDelegates) {
                if (!fieldDelegate.equals(left, right)) {
                    return false
                }
            }

            return true
        }

        inline fun newObject(crossinline getProperty: (fieldDelegate: ValdiFieldDelegate) -> Any?): Any {
            val fieldValues = fieldDelegates.arrayMap(getProperty)
            return constructor.newInstance(*fieldValues)
        }

        override fun copyObject(obj: Any): Any {
            return newObject { it.getValue(obj) }
        }

        override fun disposeObject(obj: Any) {
            for (fieldDelegate in fieldDelegates) {
                fieldDelegate.disposeHeldValue(obj)
            }
        }
    }

    class ValdiInterfaceClassDelegate: ValdiClassDelegate {
        override fun objectEquals(left: Any, right: Any): Boolean {
            return left === right
        }

        override fun copyObject(obj: Any): Any {
            throw ValdiException("@GenerateNativeInterface objects cannot be copied")
        }

        override fun disposeObject(obj: Any) {
            // no-op
        }
    }

    class ValdiUntypedClassDelegate: ValdiClassDelegate {
        override fun objectEquals(left: Any, right: Any): Boolean {
            return left === right
        }

        override fun copyObject(obj: Any): Any {
            throw ValdiException("Untpyed objects cannot be copied")
        }

        override fun disposeObject(obj: Any) {
            // no-op
        }
    }

    class ValdiEnumClassDelegate(val enumValues: Array<out Any>): ValdiClassDelegate {
        override fun objectEquals(left: Any, right: Any): Boolean {
            return left === right
        }

        override fun copyObject(obj: Any): Any {
            // Enums are immutable
            return obj
        }

        override fun disposeObject(obj: Any) {
            // no-op
        }
    }

    private val classDelegates = hashMapOf<Class<*>, ValdiClassDelegate>()

    private fun resolveConstructor(cls: Class<*>): Constructor<*> {
        val constructors = cls.declaredConstructors
        var bestConstructor: Constructor<*>? = null
        var bestConstructorParametersSize = -1

        for (constructor in constructors) {
            val parameterTypes = constructor.parameterTypes
            if (!constructor.isSynthetic && parameterTypes.size > bestConstructorParametersSize) {
                bestConstructor = constructor
                bestConstructorParametersSize = parameterTypes.size
            }
        }

        return bestConstructor ?: throw ValdiException("Could not resolve constructor for class $cls")
    }

    private fun createObjectClassDelegate(cls: Class<*>, descriptor: ValdiMarshallableObjectDescriptor): ValdiObjectClassDelegate {
        val fields = cls.declaredFields
        val fieldDelegates = fields.arrayMap {
            val asField = it as Field
            asField.isAccessible = true

            return@arrayMap when (asField.type) {
                Int::class.java -> object: ValdiFieldDelegate(asField) {
                    override fun equals(left: Any, right: Any): Boolean {
                        return field.getInt(left) == field.getInt(right)
                    }
                }
                Long::class.java -> object: ValdiFieldDelegate(asField) {
                    override fun equals(left: Any, right: Any): Boolean {
                        return field.getLong(left) == field.getLong(right)
                    }
                }
                Double::class.java -> object: ValdiFieldDelegate(asField) {
                    override fun equals(left: Any, right: Any): Boolean {
                        return field.getDouble(left) == field.getDouble(right)
                    }
                }
                Boolean::class.java -> object: ValdiFieldDelegate(asField) {
                    override fun equals(left: Any, right: Any): Boolean {
                        return field.getBoolean(left) == field.getBoolean(right)
                    }
                }
                else -> object: ValdiFieldDelegate(asField) {
                    override fun equals(left: Any, right: Any): Boolean {
                        return field.get(left) == field.get(right)
                    }

                    override fun disposeHeldValue(obj: Any): Boolean {
                        val value = field.get(obj) ?: return false
                        field.set(obj, null)
                        DisposableUtils.disposeAny(value)
                        return true
                    }
                }
            }
        }

        val constructor = resolveConstructor(cls)

        return ValdiObjectClassDelegate(this, constructor, fieldDelegates)
    }

    private fun createInterfaceClassDelegate(cls: Class<*>, descriptor: ValdiMarshallableObjectDescriptor): ValdiInterfaceClassDelegate {
        return ValdiInterfaceClassDelegate()
    }

    private fun createEnumClassDelegate(cls: Class<*>, descriptor: ValdiMarshallableObjectDescriptor): ValdiEnumClassDelegate {
        val enumValues = cls.enumConstants
        return ValdiEnumClassDelegate(enumValues)
    }

    private fun createUntypedClassDelegate(cls: Class<*>, descriptor: ValdiMarshallableObjectDescriptor): ValdiUntypedClassDelegate {
        return ValdiUntypedClassDelegate()
    }

    private fun createClassDelegate(cls: Class<*>): ValdiClassDelegate {
        val descriptor = ValdiMarshallableObjectDescriptor.getDescriptorForClass(cls)

        return when (descriptor.type) {
            ValdiMarshallableObjectDescriptor.TYPE_CLASS -> createObjectClassDelegate(cls, descriptor)
            ValdiMarshallableObjectDescriptor.TYPE_INTERFACE -> createInterfaceClassDelegate(cls, descriptor)
            ValdiMarshallableObjectDescriptor.TYPE_STRING_ENUM -> createEnumClassDelegate(cls, descriptor)
            ValdiMarshallableObjectDescriptor.TYPE_INT_ENUM -> createEnumClassDelegate(cls, descriptor)
            ValdiMarshallableObjectDescriptor.TYPE_UNTYPED -> createUntypedClassDelegate(cls, descriptor)
            else ->  throw ValdiException("Unsupported type ${descriptor.type} in ${cls}")
        }
    }

    fun getClassDelegate(cls: Class<*>): ValdiClassDelegate {
        return synchronized(classDelegates) {
            var clsDelegate = classDelegates[cls]

            if (clsDelegate != null) {
                return clsDelegate
            }
            clsDelegate = createClassDelegate(cls)
            classDelegates[cls] = clsDelegate
            clsDelegate
        }
    }
}
