package com.snap.valdi.utils

import android.content.Context
import android.util.AttributeSet
import android.util.Xml.asAttributeSet

fun attributeSetFromXml(context: Context, resNum: Int): AttributeSet? {
    try {
        val parser = context.resources.getXml(resNum)
        parser.next()
        parser.nextTag()
        return asAttributeSet(parser)
    } catch (e: Exception) {
        return null
    }
}

