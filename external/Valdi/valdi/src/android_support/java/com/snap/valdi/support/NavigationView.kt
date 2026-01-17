package com.snap.valdi.support

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.content.Context
import android.view.View
import android.view.ViewGroup
import com.snap.valdi.extensions.removeFromParentView
import com.snap.valdi.navigation.INavigatorPageConfig
import com.snap.valdi.utils.runOnMainThreadIfNeeded
import com.snap.valdi.views.ValdiView

class NavigationView(context: Context): ViewGroup(context) {

    private val rootViews = mutableListOf<View>()

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)

        for (view in rootViews) {
            if (view.visibility != View.GONE) {
                view.measure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY))
            }
        }

        setMeasuredDimension(width, height)
    }

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        for (view in rootViews) {
            if (view.visibility != View.GONE) {
                view.layout(0, 0, r - l, b - t)
            }
        }
    }

    private fun finishedTransition() {
        if (rootViews.isEmpty()) {
            return
        }

        val last = rootViews.last()
        for (view in rootViews) {
            if (view === last) {
                view.visibility = View.VISIBLE
            } else {
                view.visibility = View.GONE
            }
        }
    }

    private fun animateTransition(view: View, toTranslationX: Float, completion: () -> Unit) {
        val fromTranslation = view.translationX
        val valueAnimator = ValueAnimator.ofFloat(fromTranslation, toTranslationX)
        valueAnimator.duration = 250

        valueAnimator.addUpdateListener { view.translationX = it.animatedValue as Float }
        valueAnimator.addListener(object: AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                completion()
            }
        })

        valueAnimator.start()
    }

    fun push(view: View, animated: Boolean) {
        val previousView = rootViews.lastOrNull()
        rootViews.add(view)
        addView(view)
        requestLayout()

        if (animated) {
            view.translationX = width.toFloat()
            animateTransition(view, 0f) {
                finishedTransition()
            }

            if (previousView != null) {
                animateTransition(previousView, getSlideOffset()) {

                }
            }
        } else {
            view.translationX = 0f
            finishedTransition()
        }
    }

    fun pop(animated: Boolean, completion: (removedView: View) -> Unit): Boolean {
        if (rootViews.size < 2) {
            return false
        }

        val view = rootViews.removeAt(rootViews.size - 1)
        val previousView = rootViews.lastOrNull()

        if (animated) {
            animateTransition(view, width.toFloat()) {
                view.removeFromParentView()
                finishedTransition()
                completion(view)
            }

            if (previousView != null) {
                previousView.visibility = View.VISIBLE

                previousView.translationX = getSlideOffset()
                animateTransition(previousView, 0f) {

                }
            }
        } else {
            view.removeFromParentView()
            previousView?.translationX = 0f
            finishedTransition()
            completion(view)
        }
        return true
    }

    fun top(): View? {
        return rootViews.lastOrNull()
    }

    private fun getSlideOffset(): Float {
        return -width.toFloat() / 6f
    }

}
