package com.snap.valdi.views

import android.view.ViewGroup
import android.view.MotionEvent
import android.content.Context
import androidx.annotation.Keep
import android.widget.DatePicker
import android.util.AttributeSet
import com.snap.valdi.logger.Logger
import com.snap.valdi.context.ValdiContext
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.attributeSetFromXml
import com.snap.valdi.callable.ValdiFunction
import java.util.Calendar
import java.util.Date
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.InternedString

@Keep
open class ValdiDatePicker(context: Context) : ViewGroup(context), ValdiTouchTarget {

    var dateSeconds: Float?
        get() {
            val c = Calendar.getInstance()
            c.set(Calendar.YEAR, datePicker.getYear())
            c.set(Calendar.MONTH, datePicker.getMonth())
            c.set(Calendar.DAY_OF_MONTH, datePicker.getDayOfMonth())
            return (c.getTimeInMillis() / 1000.0).toFloat()
        }
        set(value) {
            val date: Date
            if (value == null) {
                date = Date()
            } else {
                date = Date((value * 1000).toLong())
            }
            val c = Calendar.getInstance()
            c.setTime(date)
            val year = c.get(Calendar.YEAR)
            val month = c.get(Calendar.MONTH)
            val dayOfMonth = c.get(Calendar.DAY_OF_MONTH)
            safeUpdate {
                datePicker.updateDate(year, month, dayOfMonth)
            }
        }

    var minimumDateSeconds: Float?
        get() {
            val minDate = datePicker.getMinDate()
            return (minDate / 1000.0).toFloat()
        }
        set(value) {
            if (value == null) {
                datePicker.setMinDate(Long.MIN_VALUE)
                return
            }
            datePicker.setMinDate((value * 1000).toLong())
        }

    var maximumDateSeconds: Float?
        get() {
            val maxDate = datePicker.getMaxDate()
            return (maxDate / 1000.0).toFloat()
        }
        set(value) {
            if (value == null) {
                datePicker.setMaxDate(Long.MAX_VALUE)
                return
            }
            datePicker.setMaxDate((value * 1000).toLong())
        }

    var onChangeFunction: ValdiFunction? = null

    private val datePicker = DatePicker(context, datePickerAttributeSet(context))

    /**
     * The ValdiContext to which this ValdiView belongs to.
     */
    private val valdiContext: ValdiContext?
        get() = ViewUtils.findValdiContext(this)

    private val logger: Logger?
        get() {
            return valdiContext?.runtime?.logger
        }

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
        addView(datePicker)

        val c = Calendar.getInstance()
        c.time = Date()
        val year = c.get(Calendar.YEAR)
        val month = c.get(Calendar.MONTH)
        val dayOfMonth = c.get(Calendar.DAY_OF_MONTH)

        datePicker.init(year, month, dayOfMonth, object: DatePicker.OnDateChangedListener {
            override fun onDateChanged(view: DatePicker, year: Int, monthOfYear: Int, dayOfMonth: Int) {
                if (isSettingValueCount > 0) {
                    return
                }

                ViewUtils.notifyAttributeChanged(view, dateSecondsProperty, dateSeconds)

                if (onChangeFunction == null) {
                    return
                }

                ValdiMarshaller.use {
                    val objectIndex = it.pushMap(1)
                    it.putMapPropertyOptionalDouble(dateSecondsProperty, objectIndex, dateSeconds?.toDouble())

                    onChangeFunction?.perform(it)
                }
            }
        })
        datePicker.setDescendantFocusability(DatePicker.FOCUS_BLOCK_DESCENDANTS)
    }

    companion object {
        private val dateSecondsProperty = InternedString.create("dateSeconds")

        fun datePickerAttributeSet(context: Context): AttributeSet? {
            return attributeSetFromXml(context, com.snapchat.client.R.xml.valdi_date_picker)
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
        this.datePicker.measure(
            widthMeasureSpec,
            heightMeasureSpec
        )
        val width = this.datePicker.getMeasuredWidth()
        val height = this.datePicker.getMeasuredHeight()
        setMeasuredDimension(width, height)
    }

    override fun onLayout(p0: Boolean, l: Int, t: Int, r: Int, b: Int) {
        datePicker.layout(0, 0, r - l, b - t)
    }

    override fun hasOverlappingRendering(): Boolean {
        return ViewUtils.hasOverlappingRendering(this)
    }
}
