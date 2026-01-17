package com.snap.valdi.drawables.utils

import android.content.Context
import com.snap.valdi.drawables.ValdiGradientDrawable

object ValdiGradientDrawablePool {

    private val pool = mutableListOf<ValdiGradientDrawable>()

    fun releaseDrawable(drawable: ValdiGradientDrawable): Boolean {
        drawable.drawableInfoProvider = null
        pool.add(drawable)
        return true
    }

    fun createDrawable(context: Context, borderRadiiProvider: DrawableInfoProvider): ValdiGradientDrawable {
        if (pool.isNotEmpty()) {
            val item = pool.last()
            pool.removeAt(pool.lastIndex)
            item.drawableInfoProvider = borderRadiiProvider
            return item
        }

        return ValdiGradientDrawable().also { it.drawableInfoProvider = borderRadiiProvider }
    }

}