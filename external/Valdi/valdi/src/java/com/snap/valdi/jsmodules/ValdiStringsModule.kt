package com.snap.valdi.jsmodules

import android.content.Context
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import com.snapchat.client.valdi_core.ModuleFactory

class ValdiStringsModule(
        private val context: Context
) : ModuleFactory() {

    override fun getModulePath(): String {
        return "valdi_core/src/Strings"
    }

    override fun loadModule(): Any {
        val moduleDef = HashMap<String, ValdiFunction>()
        moduleDef.put("getLocalizedString", object: ValdiFunction {
            override fun perform(marshaller: ValdiMarshaller): Boolean {
                val moduleName = marshaller.getString(0)
                val stringKey = marshaller.getString(1)
                val localizedStringKey = "${moduleName}_${stringKey}"
                val identifier = context.resources.getIdentifier(localizedStringKey, "string", context.packageName)
                if (identifier == 0) {
                    marshaller.pushString("<NOT_FOUND>")
                } else {
                    marshaller.pushString(context.resources.getString(identifier))
                }
                return true
            }
        })

        return moduleDef
    }
}
