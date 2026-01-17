package com.snap.valdi.utils

import android.util.AttributeSet

class EmptyAttributeSet: AttributeSet {

    override fun getPositionDescription(): String {
        return ""
    }

    override fun getAttributeCount(): Int {
        return 0
    }

    override fun getAttributeNameResource(index: Int): Int {
        return 0
    }

    override fun getAttributeUnsignedIntValue(namespace: String?, attribute: String?, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getAttributeUnsignedIntValue(index: Int, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getAttributeValue(index: Int): String {
        return ""
    }

    override fun getAttributeValue(namespace: String?, name: String?): String {
        return ""
    }

    override fun getAttributeIntValue(namespace: String?, attribute: String?, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getAttributeIntValue(index: Int, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getIdAttribute(): String {
        return ""
    }

    override fun getIdAttributeResourceValue(defaultValue: Int): Int {
        return defaultValue
    }

    override fun getAttributeFloatValue(namespace: String?, attribute: String?, defaultValue: Float): Float {
        return defaultValue
    }

    override fun getAttributeFloatValue(index: Int, defaultValue: Float): Float {
        return defaultValue
    }

    override fun getStyleAttribute(): Int {
        return 0
    }

    override fun getAttributeName(index: Int): String {
        return ""
    }

    override fun getAttributeListValue(namespace: String?, attribute: String?, options: Array<out String>?, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getAttributeListValue(index: Int, options: Array<out String>?, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getClassAttribute(): String {
        return ""
    }

    override fun getAttributeBooleanValue(namespace: String?, attribute: String?, defaultValue: Boolean): Boolean {
        return defaultValue
    }

    override fun getAttributeBooleanValue(index: Int, defaultValue: Boolean): Boolean {
        return defaultValue
    }

    override fun getAttributeResourceValue(namespace: String?, attribute: String?, defaultValue: Int): Int {
        return defaultValue
    }

    override fun getAttributeResourceValue(index: Int, defaultValue: Int): Int {
        return defaultValue
    }

}