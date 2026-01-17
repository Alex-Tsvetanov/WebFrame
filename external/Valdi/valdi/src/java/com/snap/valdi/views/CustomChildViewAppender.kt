package com.snap.valdi.views

import android.view.View

/**
 * If a view implements this interface, the addValdiChildView
 * method will be used instead of using the regular View's addView()
 */
interface CustomChildViewAppender {

    fun addValdiChildView(childView: View, viewIndex: Int)

}