package com.snap.valdi.schema

/**
 * Annotation added to every types generated from TypeScript's
 * @GenerateNativeEnum annotation.
 */
@Target(AnnotationTarget.CLASS)
@Retention(AnnotationRetention.RUNTIME)
annotation class ValdiEnum(
        val schema: String,
        val propertyReplacements: String = "",
        val type: ValdiEnumType,
)