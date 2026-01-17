package com.snap.valdi.schema

import androidx.annotation.Keep
import com.snap.valdi.exceptions.ValdiFatalException
import com.snap.valdi.utils.arrayMap
import java.lang.Exception
import java.lang.reflect.Method
import kotlin.reflect.KClass

@Keep
class ValdiMarshallableObjectDescriptor

private constructor(val type: Int,
                    val schema: String,
                    val proxyClass: Class<*>?,
                    val typeReferences: Array<String>?,
                    val propertyReplacements: String?) {

    companion object {

        const val TYPE_CLASS = 1
        const val TYPE_INTERFACE = 2
        const val TYPE_STRING_ENUM = 3
        const val TYPE_INT_ENUM = 4
        const val TYPE_UNTYPED = 5

        private val objectImplementsMethodCache = hashMapOf<Class<*>, MutableMap<Method, Boolean>>()

        @JvmStatic
        private fun make(type: Int,
                         schema: String,
                         proxyClass: Class<*>?,
                         typeReferences: Array<KClass<*>>,
                         propertyReplacements: String): ValdiMarshallableObjectDescriptor {
            val typeReferenceNames = typeReferences.takeIf { it.isNotEmpty() }?.arrayMap { it.java.name }
            val propertyReplacementsStr = propertyReplacements.takeIf { it.isNotEmpty() }
            return ValdiMarshallableObjectDescriptor(
                    type,
                    schema,
                    proxyClass,
                    typeReferenceNames,
                    propertyReplacementsStr)
        }

        @JvmStatic
        private fun forClass(schema: String,
                             typeReferences: Array<KClass<*>>,
                             propertyReplacements: String): ValdiMarshallableObjectDescriptor {
            return make(
                    TYPE_CLASS,
                    schema,
                    null,
                    typeReferences,
                    propertyReplacements)
        }

        @JvmStatic
        private fun forInterface(schema: String,
                                 proxyClass: Class<*>,
                                 typeReferences: Array<KClass<*>>,
                                 propertyReplacements: String): ValdiMarshallableObjectDescriptor {
            return make(
                    TYPE_INTERFACE,
                    schema,
                    proxyClass,
                    typeReferences,
                    propertyReplacements)
        }

        @JvmStatic
        private fun forStringEnum(schema: String,
                                  propertyReplacements: String): ValdiMarshallableObjectDescriptor {
            return make(
                    TYPE_STRING_ENUM,
                    schema,
                    null,
                    emptyArray(),
                    propertyReplacements)
        }

        @JvmStatic
        private fun forIntEnum(schema: String,
                               propertyReplacements: String): ValdiMarshallableObjectDescriptor {
            return make(
                    TYPE_INT_ENUM,
                    schema,
                    null,
                    emptyArray(),
                    propertyReplacements)
        }

        @JvmStatic
        private fun forUntyped(): ValdiMarshallableObjectDescriptor {
            return make(TYPE_UNTYPED, "u", null, emptyArray(), "")
        }

        @JvmStatic
        private fun forFunction(schema: String,
                                typeReferences: Array<KClass<*>>,
                                propertyReplacements: String): ValdiMarshallableObjectDescriptor {
            val resolvedPropertyReplacements = if (propertyReplacements.isNotEmpty()) propertyReplacements else "0:'_invoker'"
            return forClass(schema, typeReferences, resolvedPropertyReplacements)
        }

        @Keep
        @JvmStatic
        fun getDescriptorForClass(cls: Class<*>): ValdiMarshallableObjectDescriptor {
            try {
                if (cls.isInterface) {
                    val valdiInterface = cls.getAnnotation(ValdiInterface::class.java)
                    if (valdiInterface != null) {
                        return forInterface(
                                valdiInterface.schema,
                                valdiInterface.proxyClass.java,
                                valdiInterface.typeReferences,
                                valdiInterface.propertyReplacements)
                    }

                    val valdiUntyped = cls.getAnnotation(ValdiUntypedClass::class.java)
                    if (valdiUntyped != null) {
                        return forUntyped()
                    }
                }

                if (cls.isEnum) {
                    val valdiEnum = cls.getAnnotation(ValdiEnum::class.java)
                    if (valdiEnum != null) {
                        return when (valdiEnum.type) {
                            ValdiEnumType.INT -> forIntEnum(valdiEnum.schema, valdiEnum.propertyReplacements)
                            ValdiEnumType.STRING -> forStringEnum(valdiEnum.schema, valdiEnum.propertyReplacements)
                        }
                    }
                }

                val valdiClass = cls.getAnnotation(ValdiClass::class.java)
                if (valdiClass != null) {
                    return forClass(
                            valdiClass.schema,
                            valdiClass.typeReferences,
                            valdiClass.propertyReplacements)
                }

                val valdiFunction = cls.getAnnotation(ValdiFunctionClass::class.java)
                if (valdiFunction != null) {
                    return forFunction(
                            valdiFunction.schema,
                            valdiFunction.typeReferences,
                            valdiFunction.propertyReplacements)
                }

                throw Exception("Could not resolve Valdi Annotation")
            } catch (exc: Throwable) {
                ValdiFatalException.handleFatal(exc, "Could not resolve descriptor for class ${cls.name}")
            }
        }

        @JvmStatic
        private fun resolveClassImplementsMethod(cls: Class<*>, method: Method): Boolean {
            return try {
                val methodInClass = cls.getMethod(method.name, *method.parameterTypes)
                methodInClass.getAnnotation(ValdiOptionalMethod::class.java) == null
            } catch (exc: NoSuchMethodError) {
                false
            }
        }

        @Keep
        @JvmStatic
        fun objectImplementsMethod(obj: Any, method: Method): Boolean {
            val cls = obj.javaClass

            return synchronized(objectImplementsMethodCache) {
                var methodCache = objectImplementsMethodCache[cls]
                if (methodCache == null) {
                    methodCache = hashMapOf()
                    objectImplementsMethodCache[cls] = methodCache
                }

                var implements = methodCache[method]
                if (implements == null) {
                    implements = resolveClassImplementsMethod(cls, method)
                    methodCache[method] = implements
                }

                implements
            }
        }
    }

}