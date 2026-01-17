package com.snap.valdi.utils

import android.graphics.Canvas
import android.view.View
import android.net.Uri

import com.snap.valdi.callable.ValdiFunction

interface ValdiVideoPlayer {
    data class Callbacks(
        val onVideoLoaded: ValdiFunction? = null,
        val onBeginPlayback: ValdiFunction? = null,
        val onError: ValdiFunction? = null,
        val onCompleted: ValdiFunction? = null,
        val onProgressUpdated: ValdiFunction? = null,
    )

    fun getView(): View

    fun setRequestPayload(payload: Any?)

    fun setVolume(volume: Float)

    fun setPlaybackRate(rate: Float)

    fun setSeekToTime(time: Float)

    fun setCallbacks(callbacks: Callbacks?)
}
