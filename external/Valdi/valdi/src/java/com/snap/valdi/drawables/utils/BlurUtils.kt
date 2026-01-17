
package com.snap.valdi.drawables.utils

import android.content.Context
import android.graphics.Bitmap
import android.renderscript.Allocation
import android.renderscript.Element
import android.renderscript.RenderScript
import android.renderscript.ScriptIntrinsicBlur
import com.snap.valdi.utils.BitmapHandler
import com.snap.valdi.utils.BitmapPool

@Suppress("DEPRECATION")
object BlurUtils {

    fun blur(context: Context,
             bitmapPool: BitmapPool,
             bitmap: Bitmap,
             blurRadius: Float): BitmapHandler? {
        var rs: RenderScript? = null
        var input: Allocation? = null
        var output: Allocation? = null
        var blur: ScriptIntrinsicBlur? = null
        try {
            rs = RenderScript.create(context)
            rs.messageHandler = RenderScript.RSMessageHandler()
            input = Allocation.createFromBitmap(rs, bitmap, Allocation.MipmapControl.MIPMAP_NONE,
                    Allocation.USAGE_SCRIPT)
            output = Allocation.createTyped(rs, input.type)
            blur = ScriptIntrinsicBlur.create(rs, Element.U8_4(rs))
            blur.setInput(input)
            blur.setRadius(Math.max(Math.min(blurRadius, 25f), 0f))
            blur.forEach(output)

            val bitmapOutput = bitmapPool.get(bitmap.width, bitmap.height) ?: return null
            output.copyTo(bitmapOutput.getBitmap())

            return bitmapOutput
        } finally {
            rs?.destroy()
            input?.destroy()
            output?.destroy()
            blur?.destroy()
        }
    }
}