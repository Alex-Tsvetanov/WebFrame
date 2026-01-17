package com.snap.valdi.utils

import android.graphics.Path
import android.graphics.RectF
import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.snapdrawing.PathUtils
import java.nio.BufferUnderflowException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.max
import kotlin.math.min

class GeometricPath {

    val path: Path
        get() {
            updatePathIfNeeded()
            return innerPath
        }

    var width: Int = 0
        set(newValue) {
            if (field != newValue) {
                field = newValue
                setDirty()
            }
        }

    var height: Int = 0
        set(newValue) {
            if (field != newValue) {
                field = newValue
                setDirty()
            }
        }

    val isEmpty: Boolean
        get() = pathData == null

    private val innerPath = Path()
    private var dirty = false
    private var pathData: ByteArray? = null

    fun setPathData(pathData: ByteArray?) {
        this.pathData = pathData
        setDirty()
    }

    private fun setDirty() {
        dirty = true
    }

    private fun updatePathIfNeeded() {
        /**
         * The `synchronized(this)` here is a speculative fix for ticket 4485
         */
        synchronized(this) {
            if (dirty) {
                dirty = false
                innerPath.reset()
                val pathData = pathData
                if (pathData != null) {
                    pathDataToPath(pathData, width, height, innerPath)
                }
            }
        }
    }

    companion object {
        private fun readValue(buffer: ByteBuffer, ratio: Double, offset: Double): Float {
            return (buffer.double * ratio + offset).toFloat()
        }

        private val tmp = RectF()

        private const val SCALE_TYPE_FILL = 1
        private const val SCALE_TYPE_CONTAIN = 2
        private const val SCALE_TYPE_COVER = 3
        private const val SCALE_TYPE_NONE = 4

        fun pathDataToPath(bytes: ByteArray, width: Int, height: Int, outPath: Path) {
            val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
            try {
                val extentWidth = buffer.double
                val extentHeight = buffer.double

                if (extentWidth <= 0 || extentHeight <= 0) {
                    return
                }

                var xRatio = width.toDouble() / extentWidth
                var yRatio = height.toDouble() / extentHeight

                when (buffer.double.toInt()) {
                    SCALE_TYPE_FILL -> {
                        // Default behavior
                    }
                    SCALE_TYPE_CONTAIN -> {
                        val minRatio = min(xRatio, yRatio)
                        xRatio = minRatio
                        yRatio = minRatio
                    }
                    SCALE_TYPE_COVER -> {
                        val maxRatio = max(xRatio, yRatio)
                        xRatio = maxRatio
                        yRatio = maxRatio
                    }
                    SCALE_TYPE_NONE -> {
                        xRatio = 1.0
                        yRatio = 1.0
                    }
                    else -> throw ValdiException("Invalid scale type")
                }

                val xOffset = (width - (extentWidth * xRatio)) / 2.0
                val yOffset = (height - (extentHeight * yRatio)) / 2.0

                while (buffer.hasRemaining()) {
                    when (buffer.double.toFloat()) {
                        PathUtils.COMPONENT_MOVE -> {
                            outPath.moveTo(readValue(buffer, xRatio, xOffset), readValue(buffer, yRatio, yOffset))
                        }
                        PathUtils.COMPONENT_LINE -> {
                            outPath.lineTo(readValue(buffer, xRatio, xOffset), readValue(buffer, yRatio, yOffset))
                        }
                        PathUtils.COMPONENT_QUAD -> {
                            outPath.quadTo(
                                    readValue(buffer, xRatio, xOffset),
                                    readValue(buffer, yRatio, yOffset),
                                    readValue(buffer, xRatio, xOffset),
                                    readValue(buffer, yRatio, yOffset),
                            )
                        }
                        PathUtils.COMPONENT_CUBIC -> {
                            val x = readValue(buffer, xRatio, xOffset)
                            val y = readValue(buffer, yRatio, yOffset)
                            outPath.cubicTo(
                                    readValue(buffer, xRatio, xOffset),
                                    readValue(buffer, yRatio, yOffset),
                                    readValue(buffer, xRatio, xOffset),
                                    readValue(buffer, yRatio, yOffset),
                                    x,
                                    y
                            )
                        }
                        PathUtils.COMPONENT_ROUND_RECT -> {
                            val left = readValue(buffer, xRatio, xOffset)
                            val top = readValue(buffer, yRatio, yOffset)
                            tmp.set(left, top, left + readValue(buffer, xRatio, 0.0), top + readValue(buffer, yRatio, 0.0))
                            outPath.addRoundRect(
                                    tmp,
                                    readValue(buffer, xRatio, 0.0),
                                    readValue(buffer, yRatio, 0.0),
                                    Path.Direction.CW
                            )
                        }
                        PathUtils.COMPONENT_ARC -> {
                            val centerX = readValue(buffer, xRatio, xOffset)
                            val centerY = readValue(buffer, yRatio, yOffset)
                            val radius = readValue(buffer, 1.0, 0.0)
                            val radiusX = (radius * xRatio).toFloat()
                            val radiusY = (radius * yRatio).toFloat()
                            val startAngle = Math.toDegrees(buffer.double).toFloat()
                            val sweepAngle = Math.toDegrees(buffer.double).toFloat()

                            tmp.set(centerX - radiusX, centerY - radiusY, centerX + radiusX, centerY + radiusY)
                            outPath.addArc(
                                    tmp,
                                    startAngle,
                                    sweepAngle
                            )
                        }
                        PathUtils.COMPONENT_CLOSE -> {
                            outPath.close()
                        }
                        else -> throw ValdiException("Invalid Path")
                    }
                }
            } catch (exc: BufferUnderflowException) {
                throw ValdiException("Invalid Path")
            }
        }
    }

}
