package com.snap.valdi.utils

import kotlin.math.roundToInt

enum class QoSClass(val value: Int) {
    LOWEST(0),
    LOW(1),
    NORMAL(2),
    HIGH(3),
    MAX(4);

    fun toThreadPriority(): Int {
        val qosClassRatio = value.toDouble() / MAX.value.toDouble()
        val threadPriorityOffset = (Thread.MAX_PRIORITY - Thread.MIN_PRIORITY).toDouble() * qosClassRatio
        val threadPriority = Thread.MIN_PRIORITY + threadPriorityOffset.roundToInt()

        return Math.min(Math.max(threadPriority, Thread.MIN_PRIORITY), Thread.MAX_PRIORITY)
    }

    companion object {
        @JvmStatic
        fun fromValue(value: Int): QoSClass {
            return values().first { it.value == value }
        }
    }
}