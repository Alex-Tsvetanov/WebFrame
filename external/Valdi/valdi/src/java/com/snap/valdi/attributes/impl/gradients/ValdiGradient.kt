package com.snap.valdi.attributes.impl.gradients

import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.GradientDrawable.Orientation
import com.snap.valdi.attributes.conversions.ColorConversions
import com.snap.valdi.exceptions.AttributeError

/**
 * Class for representing a Valdi Gradient and contains method for parsing
 * the raw data that is converted from the valdi string representation.
 */
class ValdiGradient(
    colors: IntArray,
    locations: FloatArray?,
    isRadial: Boolean,
    orientation: Int = 0
) {
    enum class GradientType {
        LINEAR, RADIAL
    }

    companion object {
        private const val EXPECTED_GRADIENT_DATA_SIZE = 4

        /**
         * Reads the raw data that defines the Valdi gradient and returns
         * an instanced representation of this class.
         */
        fun fromGradientData(gradientData: Array<Any>): ValdiGradient {
            if (gradientData.size != EXPECTED_GRADIENT_DATA_SIZE) {
                throw AttributeError(
                    "Gradient should have four values in the given array: " +
                            "colors, locations, orientation, and isRadial")
            }

            val colors = fromColors((gradientData[0]))
            val locations = fromLocations(gradientData[1])
            val orientation = (gradientData[2] as? Int) ?: 0
            val isRadial = (gradientData[3] as? Boolean) ?: false

            return ValdiGradient(colors, locations, isRadial, orientation)
        }

        private fun fromColors(colorsData: Any): IntArray {
            val colors = colorsData as? Array<Any>
                ?: throw AttributeError("colors should be an array")

            return colors.map { ColorConversions.fromRGBA(it as Long) }.toIntArray()
        }

        private fun fromLocations(locationsData: Any): FloatArray? {
            val locations = locationsData as? Array<Any>
                ?: throw AttributeError("locations should be an array")

            return if (locations.isNotEmpty()) FloatArray(locations.size) { (locations[it] as Number).toFloat() } else null
        }
    }

    /**
     * Type of the gradient. Currently supports two types.
     * LINEAR: Linear gradient clamped from point A to B.
     * RADIAL: Radial gradient centered on the view with radius.
     */
    var gradientType: GradientType = if (isRadial) {
        GradientType.RADIAL
    } else {
        GradientType.LINEAR
    }
        private set

    /**
     * Color transitions for the defined gradient.
     */
    var colors: IntArray = colors
        private set

    /**
     * Location of the transitions relative to the view.
     */
    var locations: FloatArray? = locations
        private set

    /**
     * Orientation of the LINEAR gradient. Supports 8 directions.
     * @See TextViewHelper.kt
     */
    var orientation: Int = orientation
        private set

    fun getDrawableOrientation(): Orientation {
        return when (this.orientation) {
            0 -> Orientation.TOP_BOTTOM
            1 -> Orientation.TR_BL
            2 -> Orientation.RIGHT_LEFT
            3 -> Orientation.BR_TL
            4 -> Orientation.BOTTOM_TOP
            5 -> Orientation.BL_TR
            6 -> Orientation.LEFT_RIGHT
            7 -> Orientation.TL_BR
            else -> Orientation.TOP_BOTTOM
        }
    }

    fun getDrawableGradientType(): Int {
        return if (this.gradientType == GradientType.LINEAR) GradientDrawable.LINEAR_GRADIENT else GradientDrawable.RADIAL_GRADIENT
    }
}
