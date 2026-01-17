package com.snap.valdi.schema

/**
 * Annotation added to every optional method member of a generated type
 * from TypeScript's @GenerateNativeInterface annotations.
 * Used to resolve to detect whether a method was actually implemented by
 * subtypes.
 */
@Target(AnnotationTarget.FUNCTION)
@Retention(AnnotationRetention.RUNTIME)
annotation class ValdiOptionalMethod