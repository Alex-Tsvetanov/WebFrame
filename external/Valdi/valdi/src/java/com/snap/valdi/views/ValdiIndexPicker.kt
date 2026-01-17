package com.snap.valdi.views

import android.view.View
import android.view.ViewGroup
import android.view.MotionEvent
import android.graphics.Canvas
import android.content.Context
import android.graphics.drawable.Drawable
import androidx.annotation.Keep
import android.widget.NumberPicker
import android.os.Build
import android.util.AttributeSet
import com.snap.valdi.logger.Logger
import com.snap.valdi.context.ValdiContext
import com.snap.valdi.context.ValdiViewOwner
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.warn
import com.snap.valdi.utils.attributeSetFromXml
import com.snap.valdi.callable.ValdiFunction
import java.util.Calendar
import java.util.Date
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.InternedString
import kotlin.math.min
import kotlin.math.max

@Keep
open class ValdiIndexPicker(context: Context) : ViewGroup(context), ValdiTouchTarget, ValdiForegroundHolder {

    private var labels: Array<String>? = null

    var onChange: ValdiFunction? = null

    private val numberPicker = NumberPicker(context, numberPickerAttributeSet(context))

    private var isSettingValueCount = 0

    private fun safeUpdate(block: () -> Unit) {
        isSettingValueCount += 1
        try {
            block()
        } finally {
            isSettingValueCount -= 1
        }
    }

    init {
        numberPicker.setMinValue(0)
        numberPicker.setDescendantFocusability(NumberPicker.FOCUS_BLOCK_DESCENDANTS);
        addView(numberPicker)
        val self = this
        numberPicker.setOnValueChangedListener(object: NumberPicker.OnValueChangeListener {
            override fun onValueChange(view: NumberPicker, oldVal: Int, newVal: Int) {
                if (isSettingValueCount > 0) {
                    return
                }
                ViewUtils.notifyAttributeChanged(view, indexProperty, newVal)
                self.notifySelectRow(newVal)
            }
        })
    }

    fun setContent(index: Int?, labels: Array<String>?) {
        safeUpdate {
            val size = labels?.size ?: 0

            if (this.labels != labels) {
                this.labels = labels
                numberPicker.setDisplayedValues(null)
                if (labels != null && size > 0) {
                    numberPicker.setMaxValue(size - 1)
                    numberPicker.setDisplayedValues(labels)
                } else {
                    numberPicker.setMaxValue(0)
                    numberPicker.setDisplayedValues(arrayOf("--"))
                }
            }

            val newIndex = max(0, min(size - 1, index ?: 0))
            if (newIndex != numberPicker.getValue()) {
                numberPicker.setValue(newIndex)
            }
        }
    }

    private fun notifySelectRow(value: Int) {
        if (onChange == null) {
            return
        }
        ValdiMarshaller.use {
            it.pushInt(value)
            onChange?.perform(it)
        }
    }

    private var valdiForegroundField: Drawable? = null
    override var valdiForeground: Drawable?
        get() {
            return valdiForegroundField
        }
        set(value) {
            valdiForegroundField = value
        }

    override fun dispatchDraw(canvas: Canvas) {
        ViewUtils.dispatchDraw(this, canvas) {
            super.dispatchDraw(canvas)
        }
    }

    override fun processTouchEvent(event: MotionEvent): ValdiTouchEventResult {
        // We could have a more comprehensive logic here
        // we could check if the view's state actually changed after receiving the event
        // but for now we just swallow the event when dispatch succeeds
        if (this.dispatchTouchEvent(event)) {
            return ValdiTouchEventResult.ConsumeEventAndCancelOtherGestures
        }
        return ValdiTouchEventResult.IgnoreEvent
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        this.numberPicker.measure(
            widthMeasureSpec,
            heightMeasureSpec
        )
        val width = this.numberPicker.getMeasuredWidth()
        val height = this.numberPicker.getMeasuredHeight()
        setMeasuredDimension(width, height)
    }

    override fun onLayout(p0: Boolean, l: Int, t: Int, r: Int, b: Int) {
        numberPicker.layout(0, 0, r - l, b - t)
    }

    override fun hasOverlappingRendering(): Boolean {
        return ViewUtils.hasOverlappingRendering(this)
    }

    override fun verifyDrawable(who: Drawable): Boolean {
        return ViewUtils.verifyDrawable(this, who) || super.verifyDrawable(who)
    }

    companion object {
        private val indexProperty = InternedString.create("index")

        fun numberPickerAttributeSet(context: Context): AttributeSet? {
            return attributeSetFromXml(context, com.snapchat.client.R.xml.valdi_number_picker)
        }
    }
}