package com.snap.valdi.utils

import android.text.TextDirectionHeuristic
import android.text.TextDirectionHeuristics
import android.text.TextUtils
import android.view.View
import java.util.Locale

/**
 * TextDirectionHeuristic implementation that uses either FIRSTSTRONG_LTR or FIRSTSTRONG_RTL
 * depending the current locale. It thus takes in account the text content as well as the current
 * locale to resolve the text direction.
 */
object ValdiTextDirectionHeuristic: TextDirectionHeuristic {

    private var lastLocale: Locale? = null
    private var isLastLocaleRTL = false

    override fun isRtl(array: CharArray?, start: Int, count: Int): Boolean {
        return getInnerTextDirectionHeuristic().isRtl(array, start, count)
    }

    override fun isRtl(cs: CharSequence?, start: Int, count: Int): Boolean {
        return getInnerTextDirectionHeuristic().isRtl(cs, start, count)
    }

    private fun getInnerTextDirectionHeuristic(): TextDirectionHeuristic {
        return if (isCurrentLocaleRTL()) {
            TextDirectionHeuristics.FIRSTSTRONG_RTL
        } else {
            TextDirectionHeuristics.FIRSTSTRONG_LTR
        }
    }

    private fun isCurrentLocaleRTL(): Boolean {
        return synchronized(this) {
            val currentLocale = Locale.getDefault()
            if (currentLocale !== this.lastLocale) {
                val dir = TextUtils.getLayoutDirectionFromLocale(currentLocale)

                this.lastLocale = currentLocale
                this.isLastLocaleRTL = dir == View.LAYOUT_DIRECTION_RTL
            }

            this.isLastLocaleRTL
        }
    }
}