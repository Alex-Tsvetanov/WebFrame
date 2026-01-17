package com.snap.valdi.schema

/**
 * Annotation added on the primary constructor for a ValdiClass type.
 * This will be the constructor used by the marshalling code to create the object.
 */
@Target(AnnotationTarget.CONSTRUCTOR)
@Retention(AnnotationRetention.BINARY)
annotation class ValdiClassConstructor