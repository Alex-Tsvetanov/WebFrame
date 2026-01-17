package com.snap.valdi.drawables.utils

import android.graphics.Path
import android.graphics.RectF
import com.snap.valdi.utils.CoordinateResolver
import kotlin.math.min

class BorderRadii(val topLeft: Float,
                  val topRight: Float,
                  val bottomRight: Float,
                  val bottomLeft: Float,
                  val topLeftIsPercent: Boolean,
                  val topRightIsPercent: Boolean,
                  val bottomRightIsPercent: Boolean,
                  val bottomLeftIsPercent: Boolean) {

    constructor(): this(0.0f,0.0f, 0.0f, 0.0f, false, false, false, false) {}

    val hasNonNullValue: Boolean =  topLeft != 0.0f || topRight != 0.0f || bottomRight != 0.0f || bottomLeft != 0.0f

    fun getTopLeft(bounds: RectF): Float {
        return getValue(topLeft, topLeftIsPercent, bounds)
    }

    fun getTopRight(bounds: RectF): Float {
        return getValue(topRight, topRightIsPercent, bounds)
    }

    fun getBottomRight(bounds: RectF): Float {
        return getValue(bottomRight, bottomRightIsPercent, bounds)
    }

    fun getBottomLeft(bounds: RectF): Float {
        return getValue(bottomLeft, bottomLeftIsPercent, bounds)
    }

    fun addToPath(bounds: RectF, path: Path) {
        val topLeft = getTopLeft(bounds)
        val topRight = getTopRight(bounds)
        val bottomRight = getBottomRight(bounds)
        val bottomLeft = getBottomLeft(bounds)

        addToPath(bounds, topLeft, topRight, bottomRight, bottomLeft, path)
    }

    fun addToPathDownscaled(bounds: RectF, path: Path, downscaleRatio: Float) {
        val topLeft = getTopLeft(bounds) / downscaleRatio
        val topRight = getTopRight(bounds) / downscaleRatio
        val bottomRight = getBottomRight(bounds) / downscaleRatio
        val bottomLeft = getBottomLeft(bounds) / downscaleRatio

        addToPath(bounds, topLeft, topRight, bottomRight, bottomLeft, path)
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BorderRadii) return false

        return topLeft == other.topLeft &&
                topRight == other.topRight &&
                bottomRight == other.bottomRight &&
                bottomLeft == other.bottomLeft &&
                topLeftIsPercent == other.topLeftIsPercent &&
                topRightIsPercent == other.topRightIsPercent &&
                bottomRightIsPercent == other.bottomRightIsPercent &&
                bottomLeftIsPercent == other.bottomLeftIsPercent
    }

    override fun hashCode(): Int {
        var result = topLeft.hashCode()
        result = 31 * result + topRight.hashCode()
        result = 31 * result + bottomRight.hashCode()
        result = 31 * result + bottomLeft.hashCode()
        result = 31 * result + topLeftIsPercent.hashCode()
        result = 31 * result + topRightIsPercent.hashCode()
        result = 31 * result + bottomRightIsPercent.hashCode()
        result = 31 * result + bottomLeftIsPercent.hashCode()
        return result
    }

    companion object {
        @JvmStatic
        fun fromPointValues(topLeft: Float,
                            topRight: Float,
                            bottomRight: Float,
                            bottomLeft: Float,
                            topLeftIsPercent: Boolean,
                            topRightIsPercent: Boolean,
                            bottomRightIsPercent: Boolean,
                            bottomLeftIsPercent: Boolean,
                            coordinateResolver: CoordinateResolver): BorderRadii {
            val resolvedTopLeft = if (!topLeftIsPercent) coordinateResolver.toPixelF(topLeft) else topLeft
            val resolvedTopRight = if (!topRightIsPercent) coordinateResolver.toPixelF(topRight) else topLeft
            val resolvedBottomRight = if (!bottomRightIsPercent) coordinateResolver.toPixelF(bottomRight) else topLeft
            val resolvedBottomLeft = if (!bottomLeftIsPercent) coordinateResolver.toPixelF(bottomLeft) else topLeft

            return BorderRadii(resolvedTopLeft, resolvedTopRight, resolvedBottomRight, resolvedBottomLeft, topLeftIsPercent, topRightIsPercent, bottomRightIsPercent, bottomLeftIsPercent)
        }

        @JvmStatic
        fun makeCircle(radius: Float, isPercent: Boolean): BorderRadii {
            return BorderRadii(radius, radius, radius, radius, isPercent, isPercent, isPercent, isPercent)
        }

        @JvmStatic
        fun getValue(value: Float, isPercent: Boolean, bounds: RectF): Float {
            if (!isPercent) {
                return value
            }
            return value * (min(bounds.width(), bounds.height()) / 100f)
        }

        @JvmStatic
        private val tmpArray = FloatArray(8)

        @JvmStatic
        fun addToPath(bounds: RectF,
                      topLeftCornerRadius: Float,
                      topRightCornerRadius: Float,
                      bottomRightCornerRadius: Float,
                      bottomLeftCornerRadius: Float,
                      path: Path) {
            val cornerRadiusArray = tmpArray
            populateCornerRadiusArray(topLeftCornerRadius,
                    topRightCornerRadius,
                    bottomRightCornerRadius,
                    bottomLeftCornerRadius,
                    cornerRadiusArray)

            path.addRoundRect(bounds, cornerRadiusArray, Path.Direction.CW)
        }

        @JvmStatic
        fun populateCornerRadiusArray(topLeftCornerRadius: Float,
                                      topRightCornerRadius: Float,
                                      bottomRightCornerRadius: Float,
                                      bottomLeftCornerRadius: Float,
                                      cornerRadiusArray: FloatArray) {
            cornerRadiusArray[0] = topLeftCornerRadius
            cornerRadiusArray[1] = topLeftCornerRadius
            cornerRadiusArray[2] = topRightCornerRadius
            cornerRadiusArray[3] = topRightCornerRadius
            cornerRadiusArray[4] = bottomRightCornerRadius
            cornerRadiusArray[5] = bottomRightCornerRadius
            cornerRadiusArray[6] = bottomLeftCornerRadius
            cornerRadiusArray[7] = bottomLeftCornerRadius
        }
    }
}