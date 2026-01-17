package com.snap.valdi.utils

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Color
import android.graphics.Point
import android.view.WindowManager
import com.snap.valdi.logger.Logger
import java.util.*
import java.util.concurrent.atomic.AtomicInteger
import kotlin.math.max
import kotlin.math.roundToInt

/**
 * An object managing a pool of Bitmap objects.
 * It currently only temporarily holds onto the cached bitmaps,
 * releasing them after a short time has passed without any consumer
 * claiming them.
 */
class BitmapPool(context: Context, val bitmapConfig: Bitmap.Config, val logger: Logger) {

    val maxWidth: Int
    val maxHeight: Int

    private val cachedBitmaps = mutableListOf<CachedBitmap>()
    private var cleanUpTimer: Timer? = null

    init {
        val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        val displaySize = Point()
        windowManager.defaultDisplay.getRealSize(displaySize)
        maxWidth = displaySize.x
        maxHeight = displaySize.y
    }

    private class CachedBitmap(private val pool: BitmapPool, private var bitmap: Bitmap?): BitmapHandler {

        private val retainCount = AtomicInteger(1)

        var expirationTime = 0L

        private var isDirty = true

        val isUnused: Boolean
            get() = retainCount.get() == 0

        override fun getBitmap(): Bitmap {
            return bitmap!!
        }

        fun prepareForReuse() {
            synchronized(this) {
                if (!isDirty) {
                    return
                }

                isDirty = false
                bitmap?.eraseColor(Color.TRANSPARENT)
            }
        }

        fun markDirty() {
            synchronized(this) {
                isDirty = true
            }
        }

        fun destroy() {
            synchronized(this) {
                bitmap?.recycle()
                bitmap = null
            }
        }

        override fun retain() {
            retainCount.incrementAndGet()
        }

        override fun release() {
            val newValue = retainCount.decrementAndGet()
            if (newValue == 0) {
                pool.onBitmapUnused(this)
            }
        }
    }

    /**
     * Returns a bitmap with the desired width and height.
     * The actual returned bitmap size might be smaller than the requested size.
     */
    fun get(desiredWidth: Int, desiredHeight: Int): BitmapHandler? {
        if (desiredWidth == 0 || desiredHeight == 0) {
            return null
        }

        val resolvedWidth: Int
        val resolvedHeight: Int

        if (desiredWidth <= maxWidth && desiredHeight <= maxHeight) {
            resolvedWidth = desiredWidth
            resolvedHeight = desiredHeight
        } else {
            val desiredWidthF = desiredWidth.toFloat()
            val desiredHeightF = desiredHeight.toFloat()

            val xRatio = desiredWidthF / maxWidth.toFloat()
            val yRatio = desiredHeightF / maxHeight.toFloat()
            val ratio = max(xRatio, yRatio)

            resolvedWidth = (desiredWidthF / ratio).roundToInt()
            resolvedHeight = (desiredHeightF / ratio).roundToInt()
        }

        var resolvedCachedBitmap: CachedBitmap? = null

        synchronized(cachedBitmaps) {
            val it  = cachedBitmaps.iterator()
            while (it.hasNext()) {
                val cachedBitmap = it.next()

                // TODO(simon): We could leverage reconfigure() to allow re-using buffers which are
                // bigger than the requested size.
                if (cachedBitmap.getBitmap().width == resolvedWidth && cachedBitmap.getBitmap().height == resolvedHeight) {
                    cachedBitmap.retain()

                    it.remove()

                    resolvedCachedBitmap = cachedBitmap

                    break
                }
            }
        }

        if (resolvedCachedBitmap != null) {
            resolvedCachedBitmap!!.prepareForReuse()
            return resolvedCachedBitmap
        }

        return try {
//            logger.info("Allocating bitmap of size ${resolvedWidth}x${resolvedHeight}")
            val bitmap = Bitmap.createBitmap(resolvedWidth, resolvedHeight, bitmapConfig) ?: return null
            CachedBitmap(this, bitmap)
        } catch (exc: OutOfMemoryError) {
            logger.error("Failed to allocate bitmap of size ${resolvedWidth}x${resolvedHeight}: ${exc.message}")
            null
        }
    }

    fun clear() {
        synchronized(cachedBitmaps) {
            while (cachedBitmaps.isNotEmpty()) {
                val cachedBitmap = cachedBitmaps.removeAt(cachedBitmaps.lastIndex)
                cachedBitmap.destroy()
            }
        }
    }

    private fun onBitmapUnused(cachedBitmap: CachedBitmap) {
        synchronized(cachedBitmaps) {
            if (cleanUpTimer == null) {
                val timer = Timer("Valdi BitmapPool GC")
                cleanUpTimer = timer

                timer.scheduleAtFixedRate(object: TimerTask() {
                    override fun run() {
                        doCleanup()
                    }
                }, bitmapDestroyDelayMs, bitmapDestroyDelayMs)
            }

            if (cachedBitmap.isUnused && !cachedBitmaps.contains(cachedBitmap)) {
                cachedBitmaps.add(cachedBitmap)
                cachedBitmap.expirationTime = System.nanoTime() + (bitmapDestroyDelayMs * 1000000L)
                cachedBitmap.markDirty()

                cleanUpTimer?.schedule(object: TimerTask() {
                    override fun run() {
                        if (cachedBitmap.isUnused) {
                            cachedBitmap.prepareForReuse()
                        }
                    }
                }, 0L)
            }
        }
    }

    private fun doCleanup() {
        val currentTime = System.nanoTime()

        synchronized(cachedBitmaps) {
            val it  = cachedBitmaps.iterator()
            while (it.hasNext()) {
                val cachedBitmap = it.next()

                if (currentTime >= cachedBitmap.expirationTime) {
//                    logger.info("Discarding expired Bitmap of size ${cachedBitmap.getBitmap().width}x${cachedBitmap.getBitmap().height}")
                    it.remove()
                    cachedBitmap.destroy()
                }
            }

            if (cachedBitmaps.isEmpty()) {
                cleanUpTimer?.cancel()
                cleanUpTimer = null
            }
        }
    }

    companion object {
        // How long we keep the bitmaps alive after they were released
        const val bitmapDestroyDelayMs = 2000L
    }

}