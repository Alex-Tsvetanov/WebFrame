package com.snap.valdi.schema

/**
 * Annotation added to types that should be marshalled as untyped.
 * Will be at some point removed.
 */
@Target(AnnotationTarget.CLASS)
@Retention(AnnotationRetention.RUNTIME)
annotation class ValdiUntypedClass