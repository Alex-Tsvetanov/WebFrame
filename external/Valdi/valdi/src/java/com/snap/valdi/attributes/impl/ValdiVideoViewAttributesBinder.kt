package com.snap.valdi.attributes.impl

import android.content.Context
import android.graphics.Color
import android.widget.ImageView
import com.snap.valdi.attributes.AttributesBinder
import com.snap.valdi.attributes.AttributesBindingContext
import com.snap.valdi.attributes.impl.animations.ValdiAnimator
import com.snap.valdi.exceptions.AttributeError
import com.snap.valdi.views.ValdiVideoView
import com.snapchat.client.valdi_core.AssetOutputType
import com.snap.valdi.callable.ValdiFunction

class ValdiVideoViewAttributesBinder(val context: Context) :
    AttributesBinder<ValdiVideoView> {

    override val viewClass: Class<ValdiVideoView>
        get() = ValdiVideoView::class.java

    fun applyVolume(view: ValdiVideoView, value: Float, animator: ValdiAnimator?) {
        view.volume = value
    }

    fun resetVolume(view: ValdiVideoView, animator: ValdiAnimator?) {
        view.volume = 1f
    }

    fun applyPlaybackRate(view: ValdiVideoView, value: Float, animator: ValdiAnimator?) {
        view.playbackRate = value
    }

    fun resetPlaybackRate(view: ValdiVideoView, animator: ValdiAnimator?) {
        view.playbackRate = 0f
    }

    fun applySeekToTime(view: ValdiVideoView, value: Float, animator: ValdiAnimator?) {
        view.seekToTime = value
    }

    fun resetSeekToTime(view: ValdiVideoView, animator: ValdiAnimator?) {
        view.seekToTime = 0f
    }

    fun applyOnVideoLoaded(view: ValdiVideoView, fn: ValdiFunction) {
        view.setOnVideoLoadedCallback(fn)
    }

    fun resetOnVideoLoaded(view: ValdiVideoView) {
        view.setOnVideoLoadedCallback(null)
    }

    fun applyOnBeginPlaying(view: ValdiVideoView, fn: ValdiFunction) {
        view.setOnBeginPlayingCallback(fn)
    }

    fun resetOnBeginPlaying(view: ValdiVideoView) {
        view.setOnBeginPlayingCallback(null)
    }

    fun applyOnError(view: ValdiVideoView, fn: ValdiFunction) {
        view.setOnErrorCallback(fn)
    }

    fun resetOnError(view: ValdiVideoView) {
        view.setOnErrorCallback(null)
    }

    fun applyOnCompleted(view: ValdiVideoView, fn: ValdiFunction) {
        view.setOnCompletedCallback(fn)
    }

    fun resetOnCompleted(view: ValdiVideoView) {
        view.setOnCompletedCallback(null)
    }

    fun applyOnProgressUpdated(view: ValdiVideoView, fn: ValdiFunction) {
        view.setOnProgressUpdatedCallback(fn)
    }

    fun resetOnProgressUpdated(view: ValdiVideoView) {
        view.setOnProgressUpdatedCallback(null)
    }

    override fun bindAttributes(attributesBindingContext: AttributesBindingContext<ValdiVideoView>) {
        attributesBindingContext.bindAssetAttributes(AssetOutputType.VIDEOANDROID)

        attributesBindingContext.bindFloatAttribute("volume", false, this::applyVolume, this::resetVolume)
        attributesBindingContext.bindFloatAttribute("playbackRate", false, this::applyPlaybackRate, this::resetPlaybackRate)
        attributesBindingContext.bindFloatAttribute("seekToTime", false, this::applySeekToTime, this::resetSeekToTime);

        attributesBindingContext.bindFunctionAttribute(
            "onVideoLoaded",
            this::applyOnVideoLoaded,
            this::resetOnVideoLoaded
        )
        attributesBindingContext.bindFunctionAttribute(
            "onBeginPlaying",
            this::applyOnBeginPlaying,
            this::resetOnBeginPlaying
        )
        attributesBindingContext.bindFunctionAttribute(
            "onError",
            this::applyOnError,
            this::resetOnError
        )
        attributesBindingContext.bindFunctionAttribute(
            "onCompleted",
            this::applyOnCompleted,
            this::resetOnCompleted
        )
        attributesBindingContext.bindFunctionAttribute(
            "onProgressUpdated",
            this::applyOnProgressUpdated,
            this::resetOnProgressUpdated
        )

    }

}
