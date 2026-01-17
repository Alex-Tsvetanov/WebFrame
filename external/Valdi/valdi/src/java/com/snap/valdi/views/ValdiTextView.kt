package com.snap.valdi.views

import android.content.Context
import android.text.TextDirectionHeuristic
import android.widget.TextView
import com.snap.valdi.attributes.impl.richtext.TextViewHelper
import com.snap.valdi.utils.trace

class ValdiTextView(context: Context) : TextView(context), ValdiRecyclableView, ValdiTextHolder {

    override var textViewHelper: TextViewHelper? = null

    init {
        TextViewUtils.configure(this)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        trace({"ValdiTextView.onMeasure"}) {
            textViewHelper?.onMeasure(widthMeasureSpec, heightMeasureSpec)
            super.onMeasure(widthMeasureSpec, TextViewUtils.resolveHeightMeasureSpec(this, heightMeasureSpec))
        }
    }

    override fun getTextDirectionHeuristic(): TextDirectionHeuristic {
        return TextViewUtils.resolveTextDirectionHeuristic(super.getTextDirectionHeuristic())
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        textViewHelper?.onLayout(changed)
        super.onLayout(changed, left, top, right, bottom)
    }

    override fun setTextAccessibility(text: CharSequence?) {
        super.setText(text, null)
    }
}
