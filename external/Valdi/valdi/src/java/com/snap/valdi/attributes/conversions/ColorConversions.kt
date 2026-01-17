package com.snap.valdi.attributes.conversions

import android.graphics.Color

class ColorConversions {

    companion object {

        /**
         * Convert a Valdi color in RGBA to Android's ARGB
         */
        fun fromRGBA(rgba: Long): Int {
            return Color.argb((rgba and 0xFF).toInt(),
                    (rgba shr 24 and 0xFF).toInt(),
                    (rgba shr 16 and 0xFF).toInt(),
                    (rgba shr 8 and 0xFF).toInt())
        }

        /**
         * Convert an Android ARGB Color to a Valdi RGBA color
         */
        fun toRGBA(color: Int): Long {
            val alpha = Color.alpha(color).toLong()
            val red = Color.red(color).toLong()
            val green = Color.green(color).toLong()
            val blue = Color.blue(color).toLong()

            return alpha or (blue shl 8) or (green shl 16) or (red shl 24)
        }
    }

}

