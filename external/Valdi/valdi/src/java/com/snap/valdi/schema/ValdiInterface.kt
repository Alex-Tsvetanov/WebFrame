package com.snap.valdi.schema

import kotlin.reflect.KClass

/**
 * Annotation added to every types generated from TypeScript's
 * @GenerateNativeInterface annotation.
 */
@Target(AnnotationTarget.CLASS)
@Retention(AnnotationRetention.RUNTIME)
annotation class ValdiInterface(
        val schema: String,
        /**
         * An array containing the foreign type references used in the schema.
         * The final schema will be computed using the class names from this type references.
         */
        val typeReferences: Array<KClass<*>> = [],
        val propertyReplacements: String = "",
        /**
         * The proxy class that implements this interface, and will be used
         * when a proxy instance needs to be created.
         */
        val proxyClass: KClass<*>,
)