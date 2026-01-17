package com.snap.valdi.views

import android.view.View
import android.view.ViewGroup
import android.view.MotionEvent
import android.content.Context
import android.content.res.Resources
import androidx.annotation.Keep
import android.widget.DatePicker
import android.widget.NumberPicker
import android.widget.TimePicker
import android.util.Xml.asAttributeSet
import android.os.Build
import android.util.AttributeSet
import android.text.format.DateFormat
import com.snap.valdi.logger.Logger
import com.snap.valdi.context.ValdiContext
import com.snap.valdi.context.ValdiViewOwner
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.error
import com.snap.valdi.utils.warn
import com.snap.valdi.utils.attributeSetFromXml
import com.snap.valdi.callable.ValdiFunction
import com.snap.valdi.utils.ValdiMarshaller
import com.snap.valdi.utils.InternedString

@Keep
open class ValdiTimePicker(context: Context) : ViewGroup(context), ValdiTouchTarget {

    var hourOfDay: Int?
        get() {
            return underlyingTimePickerHour
        }
        set(value) {
            safeUpdate {
                underlyingTimePickerHour = value ?: 0
            }
        }

    var minuteOfHour: Int?
        get() {
            return underlyingTimePickerMinuteIndex * intervalMinutes
        }
        set(value) {
            safeUpdate {
                val min = (value ?: 0) / intervalMinutes
                underlyingTimePickerMinuteIndex = min ?: 0
            }
        }

    var intervalMinutes: Int = 1
        set(value) {
            safeUpdate {
                setMinutesInterval(value)
                field = value
            }
        }
        

    private var underlyingTimePickerHour: Int
        get() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                return timePicker.getHour()
            } else {
                return timePicker.getCurrentHour()
            }
        }
        set(value) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                timePicker.setHour(value)
            } else {
                timePicker.setCurrentHour(value)
            }
        }

    // the index of the time picker minute, multiplied by the minute interval
    private var underlyingTimePickerMinuteIndex: Int
        get() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                return timePicker.getMinute()
            } else {
                return timePicker.getCurrentMinute()
            }
        }
        set(value) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                timePicker.setMinute(value)
            } else {
                timePicker.setCurrentMinute(value)
            }
        }

    var onChangeFunction: ValdiFunction? = null

    val timePicker = TimePicker(context, timePickerAttributeSet(context))

    val valdiContext: ValdiContext?
        get() = ViewUtils.findValdiContext(this)

    val logger: Logger?
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
        addView(timePicker)
        timePicker.setIs24HourView(DateFormat.is24HourFormat(context));
        timePicker.setOnTimeChangedListener(object: TimePicker.OnTimeChangedListener {
            override fun onTimeChanged(view: TimePicker, hourOfDay: Int, minute: Int) {
                if (isSettingValueCount > 0) {
                    return
                }

                ViewUtils.notifyAttributeChanged(view, hourOfDayProperty, hourOfDay)
                ViewUtils.notifyAttributeChanged(view, minuteOfHourProperty, minute * intervalMinutes)

                if (onChangeFunction == null) {
                    return
                }

                ValdiMarshaller.use {
                    val objectIndex = it.pushMap(2)
                    it.putMapPropertyOptionalDouble(hourOfDayProperty, objectIndex, hourOfDay.toDouble())
                    it.putMapPropertyOptionalDouble(minuteOfHourProperty, objectIndex, (minute * intervalMinutes).toDouble())
                    onChangeFunction?.perform(it)
                }
            }
        })
        timePicker.setDescendantFocusability(TimePicker.FOCUS_BLOCK_DESCENDANTS)
    }

    companion object {
        private val hourOfDayProperty = InternedString.create("hourOfDay")
        private val minuteOfHourProperty = InternedString.create("minuteOfHour")

        fun timePickerAttributeSet(context: Context): AttributeSet? {
            return attributeSetFromXml(context, com.snapchat.client.R.xml.valdi_time_picker)
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
        this.timePicker.measure(
            widthMeasureSpec,
            heightMeasureSpec
        )
        val width = this.timePicker.getMeasuredWidth()
        val height = this.timePicker.getMeasuredHeight()
        setMeasuredDimension(width, height)
    }

    override fun onLayout(p0: Boolean, l: Int, t: Int, r: Int, b: Int) {
        timePicker.layout(0, 0, r - l, b - t)
    }

    override fun hasOverlappingRendering(): Boolean {
        return ViewUtils.hasOverlappingRendering(this)
    }

    private fun setMinutesInterval(intervalMinutes: Int) {
        if (intervalMinutes == 1) return
        timePicker.post {
            try {
                val minutePicker: NumberPicker = timePicker.findViewById(Resources.getSystem().getIdentifier(
                    "minute", "id", "android")) as NumberPicker
                minutePicker.minValue = 0
                minutePicker.maxValue = 60 / intervalMinutes - 1
                val displayedValues: MutableList<String> = ArrayList()
                var i = 0
                while (i < 60) {
                    displayedValues.add(String.format("%02d", i))
                    i += intervalMinutes
                }
                minutePicker.displayedValues = displayedValues.toTypedArray()
            } catch (e: Exception) {
                logger?.error("Failed to set minute interval increment for timepicker ${e}")
            }
        }
    }

}
