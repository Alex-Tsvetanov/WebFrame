package com.snap.valdi.views

import android.content.Context
import android.graphics.Canvas
import android.os.Build
import android.widget.EdgeEffect
import kotlin.math.absoluteValue

enum class Edge {
    LEFT,
    TOP,
    RIGHT,
    BOTTOM
}

class EdgeEffectWrapper(private val context: Context, private val edge: Edge) {

    private var edgeEffect: EdgeEffect? = null

    val isFinished: Boolean
        get() = edgeEffect?.isFinished ?: true

    private var lastDistance = 0f
    private var isPulling = false
    private var isReleasing = true
    private var effectWidth = 0
    private var effectHeight = 0

    private fun prepareCanvas(canvas: Canvas, boundsWidth: Int, boundsHeight: Int) {
        when (edge) {
            Edge.LEFT -> {
                canvas.rotate(270f)
                canvas.translate(-boundsHeight.toFloat(), 0f)
            }
            Edge.TOP -> {
            }
            Edge.RIGHT -> {
                canvas.rotate(90f)
                canvas.translate(0f, -boundsWidth.toFloat())
            }
            Edge.BOTTOM -> {
                canvas.rotate(180f)
                canvas.translate(-boundsWidth.toFloat(), -boundsHeight.toFloat())
            }
        }
    }

    private fun getEdge(): EdgeEffect {
        if (edgeEffect == null) {
            edgeEffect = EdgeEffect(context)
            edgeEffect!!.setSize(effectWidth, effectHeight)
        }
        return edgeEffect!!
    }

    fun draw(canvas: Canvas, boundsWidth: Int, boundsHeight: Int): Boolean {
        val unwrappedEdgeEffect = edgeEffect ?: return false

        val restore = canvas.save()
        prepareCanvas(canvas, boundsWidth, boundsHeight)
        val shouldKeepDrawing = unwrappedEdgeEffect.draw(canvas)
        canvas.restoreToCount(restore)
        if (!shouldKeepDrawing || unwrappedEdgeEffect.isFinished) {
            finish()
        }
        return shouldKeepDrawing
    }

    fun setSize(width: Int, height: Int) {
        val effectiveWidth: Int
        val effectiveHeight: Int

        when (edge) {
            Edge.LEFT, Edge.RIGHT -> {
                effectiveWidth = height
                effectiveHeight = width
            }
            Edge.TOP, Edge.BOTTOM -> {
                effectiveWidth = width
                effectiveHeight = height
            }
        }
        edgeEffect?.setSize(effectiveWidth, effectHeight)
        effectWidth = effectiveWidth
        effectHeight = effectiveHeight
    }

    fun onRelease() {
        lastDistance = 0f
        isPulling = false
        isReleasing = true
        edgeEffect?.onRelease()
    }

    fun onPull(absoluteDistance: Float, absoluteDisplacement: Float) {
        if (!isPulling) {
            finish()
            isPulling = true
        }

        val deltaDistance = absoluteDistance - lastDistance
        lastDistance = absoluteDistance
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // Adjust displacement on left and bottom since we rotate the canvas
            val adjustedDisplacement = if (edge == Edge.LEFT || edge == Edge.BOTTOM) 1f - absoluteDisplacement else absoluteDisplacement
            getEdge().onPull(deltaDistance, adjustedDisplacement)
        } else {
            getEdge().onPull(deltaDistance)
        }
    }

    fun onAbsorb(velocity: Int) {
        if (isPulling || isReleasing) {
            finish()
        }

        getEdge().onAbsorb(velocity.absoluteValue)
    }

    fun finish() {
        lastDistance = 0f
        isPulling = false
        isReleasing = false
        edgeEffect?.finish()
    }

}