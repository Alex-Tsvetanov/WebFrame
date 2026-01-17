package com.snap.valdi.schema

/**
 * Annotation added to every field member of a generated type
 * from TypeScript's @GenerateNativeClass or @GenerateNativeEnum annotations.
 */
@Target(AnnotationTarget.FIELD, AnnotationTarget.PROPERTY_GETTER, AnnotationTarget.FUNCTION)
@Retention(AnnotationRetention.BINARY)
annotation class ValdiField