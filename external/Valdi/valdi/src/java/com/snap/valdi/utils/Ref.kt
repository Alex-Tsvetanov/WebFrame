package com.snap.valdi.utils

import com.snap.valdi.schema.ValdiUntypedClass

@ValdiUntypedClass
interface Ref {
    fun get(): Any?
}