package com.snap.valdi.attributes

interface ViewLayoutAttributes {

    fun getValue(attributeName: String): Any?
    fun getBoolValue(attributeName: String): Boolean
    fun getStringValue(attributeName: String): String?
    fun getDoubleValue(attributeName: String): Double

}