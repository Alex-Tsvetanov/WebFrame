package com.snap.valdi.utils

import android.content.Context
import kotlin.math.roundToInt

/**
 * Converts coordinates from and to pixel independent units into raw pixels
 */
class CoordinateResolver(pixelDensity: Float) {

    private val pixelDensityF = pixelDensity
    private val pixelDensityD = pixelDensityF.toDouble()

    constructor(context: Context) : this(context.resources.displayMetrics.density)

    fun toPixel(point: Float): Int {
        return (point * pixelDensityF).roundToInt()
    }

    fun toPixel(point: Double): Int {
        return (point * pixelDensityD).roundToInt()
    }

    fun toPixelF(point: Float): Float {
        return (point * pixelDensityF)
    }

    fun toPixelF(point: Double): Float {
        return (point * pixelDensityD).toFloat()
    }

    fun fromPixel(pixel: Int): Double {
        return pixel.toDouble() / pixelDensityD
    }

    fun fromPixel(pixel: Float): Double {
        return pixel.toDouble() / pixelDensityD
    }

    fun fromPixel(pixel: Double): Double {
        return pixel / pixelDensityD
    }

}
