package com.snap.valdi.drawables.utils

import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.PorterDuff
import android.graphics.PorterDuffXfermode
import android.os.Build
import com.snap.valdi.utils.GeometricPath
import kotlin.math.max
import kotlin.math.min

class MaskPathRenderer {

    val isEmpty: Boolean
        get() = geometricPath.isEmpty

    private val geometricPath = GeometricPath()
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        xfermode = PorterDuffXfermode(PorterDuff.Mode.DST_OUT)
        color = Color.BLACK
    }

    fun setOpacity(opacity: Float) {
        val color =  min(max(0, (opacity * 255f).toInt()), 255) shl 24
        paint.color = color
    }

    fun setPathData(pathData: ByteArray?) {
        geometricPath.setPathData(pathData)
    }

    fun prepareMask(width: Int, height: Int, canvas: Canvas) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            canvas.saveLayer(0f, 0f, width.toFloat(), height.toFloat(), null)
        }

        geometricPath.width = width
        geometricPath.height = height
    }

    fun applyMask(canvas: Canvas) {
        canvas.drawPath(geometricPath.path, paint)
    }
}
