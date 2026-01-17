package com.snap.valdi.attributes.impl.fonts

import android.content.Context
import android.graphics.Typeface
import androidx.core.content.res.ResourcesCompat

class DefaultTypefaceResLoader: TypefaceResLoader {
    override fun loadTypeface(context: Context, resId: Int): Typeface {
        return synchronized(this) {
            ResourcesCompat.getFont(context, resId)!!
        }
    }
}