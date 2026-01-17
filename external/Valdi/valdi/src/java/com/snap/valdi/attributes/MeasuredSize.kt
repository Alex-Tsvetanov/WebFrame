package com.snap.valdi.attributes

import com.snap.valdi.nodes.ValdiViewNode

@JvmInline
value class MeasuredSize(val value: Long) {

    constructor(width: Int, height: Int): this(ValdiViewNode.encodeHorizontalVerticalPair(width, height))

    fun width(): Int {
        return ValdiViewNode.horizontalFromEncodedLong(value)
    }

    fun height(): Int {
        return ValdiViewNode.verticalFromEncodedLong(value)
    }
}