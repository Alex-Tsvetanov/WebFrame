package com.snap.valdi.attributes.impl.fonts

import android.graphics.Typeface
import com.snap.valdi.utils.LoadCompletion

interface FontLoader {

    fun load(completion: LoadCompletion<Typeface>)

    fun allowSynchronousLoad(): Boolean {
        return false
    }

}