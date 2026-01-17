package com.snap.valdi.keyboard

import android.content.Context
import android.view.View
import android.view.inputmethod.InputMethodManager

class KeyboardManager(private val rootView: View) {
    private val inputMethodManager = rootView.context.getSystemService(Context.INPUT_METHOD_SERVICE) as? InputMethodManager

    var viewRequestingFocus: View? = null

    fun requestFocusAndShowKeyboard(view: View) {
        this.onRequestFocus(view) {
            if (view.requestFocus()) {
                doShowKeyboard(view)
            }
        }
    }

    inline fun <T> onRequestFocus(view: View, cb: () -> T): T {
        val previousViewRequestingFocus = this.viewRequestingFocus
        try {
            this.viewRequestingFocus = view

            return cb()
        } finally {
            this.viewRequestingFocus = previousViewRequestingFocus
        }
    }

    fun hideKeyboard(view: View) {
        if (viewRequestingFocus == null) {
            inputMethodManager?.hideSoftInputFromWindow(
                rootView.windowToken,
                InputMethodManager.HIDE_NOT_ALWAYS
            )
        }
    }

    private fun doShowKeyboard(view: View) {
        postOnVisible(view) {
            inputMethodManager?.showSoftInput(view, InputMethodManager.SHOW_IMPLICIT)
        }
    }

    private fun postOnVisible(view: View, block: () -> Unit) {
        if (view.windowVisibility == View.GONE) {
            view.post { block() }
        } else {
            block()
        }
    }
}