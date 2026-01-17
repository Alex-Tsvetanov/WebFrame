package com.snap.valdi.attributes.impl.fonts

import android.content.Context
import android.graphics.Typeface

interface TypefaceResLoader {
    fun loadTypeface(context: Context, resId: Int): Typeface
}