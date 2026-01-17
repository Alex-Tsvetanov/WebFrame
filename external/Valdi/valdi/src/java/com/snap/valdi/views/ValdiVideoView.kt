package com.snap.valdi.views

import android.content.Context
import android.media.MediaPlayer
import android.net.Uri
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import com.snapchat.client.valdi_core.Asset
import com.snap.valdi.logger.Logger
import com.snap.valdi.utils.warn
import com.snap.valdi.extensions.ViewUtils
import com.snap.valdi.utils.CanvasClipper
import com.snap.valdi.utils.ValdiVideoPlayer
import com.snap.valdi.utils.ValdiImage
import com.snap.valdi.callable.ValdiFunction


/**
 * Base implementation for an efficient ImageView.
 * Need to be subclassed to provide an ImageLoader instance.
 */
open class ValdiVideoView(context: Context): FrameLayout(context),
    ValdiRecyclableView,
    ValdiAssetReceiver {

    /**
     * Dependencies
     */

    private var callbacks = ValdiVideoPlayer.Callbacks()
        set(value) {
            field = value
            currentPlayer?.setCallbacks(field)
        }

    var src: String? = null
    var volume: Float? = 1f
        set(value) {
            field = value
            currentPlayer?.setVolume(value ?: 1f)
        }
    private var pendingSeekToTime: Float = -1f
    var seekToTime: Float? = 0f
        set(value) {
            field = value
            val player = currentPlayer
            if (player != null) {
                player.setSeekToTime(value ?: 0f)
                pendingSeekToTime = -1f
            } else {
                if (value != null) {
                    pendingSeekToTime = value
                }
            }
        }
    var playbackRate: Float? = 0f
        set(value) {
            field = value
            currentPlayer?.setPlaybackRate(value ?: 0f)
        }


    /**
     * State properties
     */

    override fun hasOverlappingRendering(): Boolean {
        return true
    }

    /**
     * Setters for the video player
     */

    fun setOnVideoLoadedCallback(fn: ValdiFunction?) {
        callbacks = callbacks.copy(onVideoLoaded = fn)
    }

    fun setOnBeginPlayingCallback(fn: ValdiFunction?) {
        callbacks = callbacks.copy(onBeginPlayback = fn)
    }

    fun setOnErrorCallback(fn: ValdiFunction?) {
        callbacks = callbacks.copy(onError = fn)
    }

    fun setOnCompletedCallback(fn: ValdiFunction?) {
        callbacks = callbacks.copy(onCompleted = fn)
    }

    fun setOnProgressUpdatedCallback(fn: ValdiFunction?) {
        callbacks = callbacks.copy(onProgressUpdated = fn)
    }

    private var currentPlayer: ValdiVideoPlayer? = null

    override fun onAssetChanged(asset: Any?, shouldFlip: Boolean) {
        var videoPlayer = asset as? ValdiVideoPlayer
        if (videoPlayer == currentPlayer) {
            return
        }

        // detach old player
        currentPlayer?.let { player ->
            player.setCallbacks(null)

            player.setPlaybackRate(0f)
            removeView(player.getView())
        }

        currentPlayer = videoPlayer

        if (videoPlayer == null) {
            return
        }

        // attach new player
        var newView = videoPlayer.getView()
        if (newView.getParent() != this) {
            // Remove from old parent if not this.
            (newView.getParent() as? ViewGroup)?.removeView(newView)

            newView.setLayoutParams(FrameLayout.LayoutParams(FrameLayout.LayoutParams.FILL_PARENT,
                FrameLayout.LayoutParams.FILL_PARENT))
            addView(newView)
        }

        // configure new player
        videoPlayer.setCallbacks(callbacks)

        videoPlayer.setPlaybackRate(playbackRate ?: -1f)
        videoPlayer.setVolume(volume ?: -1f)
        if (pendingSeekToTime >= 0.0f) {
            videoPlayer.setSeekToTime(pendingSeekToTime)
            pendingSeekToTime = -1f
        }
    }
}
