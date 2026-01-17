package com.snap.valdi.views

import android.content.Context
import android.text.InputType
import android.view.Gravity
import androidx.annotation.Keep

@Keep
class ValdiEditTextMultiline(context: Context) : ValdiEditText(context) {

    init {
        allowLineReturns(true)
        closesWhenReturnKeyPressedDefault = false
        closesWhenReturnKeyPressed = false
        gravity = Gravity.CENTER_VERTICAL
    }

    override fun onTextChanged(text: CharSequence, start: Int, lengthBefore: Int, lengthAfter: Int) {
        super.onTextChanged(text, start, lengthBefore, lengthAfter)

        if (isSettingTextCount == 0) {
            val end = start + lengthAfter - 1
            if (end >= 0 && text.length > end && text.get(end) == '\n') {
                this.onPressedReturn()
            }
        }
    }

    fun allowLineReturns(value: Boolean) {
        if (value) {
            inputType = inputType or InputType.TYPE_TEXT_FLAG_MULTI_LINE
            maxLines = Int.MAX_VALUE
            setHorizontallyScrolling(false)
            setIgnoreNewlines(false)
        } else {
            inputType = inputType and InputType.TYPE_TEXT_FLAG_MULTI_LINE.inv()
            maxLines = Int.MAX_VALUE
            setHorizontallyScrolling(false)
            setIgnoreNewlines(true)
        }
    }

}
