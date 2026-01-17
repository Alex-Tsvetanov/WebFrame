package com.snap.valdi.attributes.impl.fonts

import android.graphics.Typeface
import com.snap.valdi.utils.LoadCompletion

object DefaultFonts {

    private fun registerDefault(fontManager: FontManager, descriptor: FontDescriptor, style: Int) {
        fontManager.register(descriptor, object: FontLoader {
            override fun load(completion: LoadCompletion<Typeface>) {
                val typeface = Typeface.defaultFromStyle(style)
                completion.onSuccess(typeface)
            }
        })
    }

    private fun registerIfExists(fontManager: FontManager,
                                 descriptor: FontDescriptor,
                                 resName: String) {
        val context = fontManager.context
        val identifier = context.resources.getIdentifier(resName, "font", context.packageName)
        if (identifier != 0) {
            fontManager.loadSyncAndRegister(descriptor, context, identifier)
        }
    }

    fun register(fontManager: FontManager) {
        registerDefault(fontManager, FontDescriptor(name = "body", family = "default"), Typeface.NORMAL)
        registerDefault(fontManager, FontDescriptor(name = "title1", family = "default"), Typeface.NORMAL)
        registerDefault(fontManager, FontDescriptor(name = "title2", family = "default"), Typeface.NORMAL)
        registerDefault(fontManager, FontDescriptor(name = "title3", family = "default", weight = FontWeight.BOLD), Typeface.BOLD)
        registerDefault(fontManager, FontDescriptor(style = FontStyle.ITALIC, family = "default"), Typeface.ITALIC)
        registerDefault(fontManager, FontDescriptor(weight = FontWeight.BOLD, style = FontStyle.ITALIC, family = "default"), Typeface.BOLD_ITALIC)

        registerIfExists(fontManager, FontDescriptor(name = "menlo-regular", family = "menlo", weight = FontWeight.NORMAL), "menlo_regular")
        registerIfExists(fontManager, FontDescriptor(name = "menlo-bold", family = "menlo", weight = FontWeight.BOLD), "menlo_bold")
    }

}