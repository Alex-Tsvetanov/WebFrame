package com.snap.valdi.support

import android.graphics.Typeface
import com.snap.valdi.ValdiRuntimeManager
import com.snap.valdi.attributes.impl.fonts.FontDescriptor
import com.snap.valdi.attributes.impl.fonts.FontWeight
import com.snap.valdi.support.R

object SupportFonts {

    @JvmStatic
    fun registerFonts(manager: ValdiRuntimeManager) {
        val fontManager = manager.fontManager
        val context = manager.context

        val regular = FontDescriptor(name = "avenirnext-regular",
                family = "avenir next",
                weight = FontWeight.NORMAL)
        fontManager.loadSyncAndRegister(regular, context, R.font.avenir_next_regular)

        val medium = FontDescriptor(name = "avenirnext-medium",
                family = "avenir next",
                weight = FontWeight.MEDIUM)
        fontManager.loadSyncAndRegister(medium, context, R.font.avenir_next_medium)

        val bold = FontDescriptor(name = "avenirnext-bold",
                family = "avenir next",
                weight = FontWeight.BOLD)
        fontManager.loadSyncAndRegister(bold, context, R.font.avenir_next_bold)

        val demiBold = FontDescriptor(name = "avenirnext-demibold",
                family = "avenir next",
                weight = FontWeight.DEMI_BOLD)
        fontManager.loadSyncAndRegister(demiBold, context, R.font.avenir_next_demi_bold)

        fontManager.register(FontDescriptor("system"), Typeface.DEFAULT)
        fontManager.register(FontDescriptor("system-bold"), Typeface.DEFAULT_BOLD)
    }
}
