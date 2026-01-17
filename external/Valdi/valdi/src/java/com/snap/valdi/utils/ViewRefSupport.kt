package com.snap.valdi.utils

import android.graphics.Canvas
import com.snap.valdi.logger.Logger
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class ViewRefSupport(val logger: Logger,
                     val bitmapPool: BitmapPool,
                     val coordinateResolver: CoordinateResolver) {

    private var renderCanvas: Canvas? = null
    private var snapshotExecutor: ExecutorService? = null

    fun getSnapshotExecutor(): ExecutorService {
        var executor = this.snapshotExecutor
        if (executor == null) {
            executor = ExecutorsUtil.newSingleThreadCachedExecutor { r ->
                Thread(r).apply {
                    name = "Valdi Snapshot Executor"
                    priority = Thread.MAX_PRIORITY
                }
            }

            this.snapshotExecutor = executor
        }

        return executor
    }

    fun getRenderCanvas(): Canvas {
        var renderCanvas = this.renderCanvas
        if (renderCanvas == null) {
            renderCanvas = Canvas()
            this.renderCanvas = renderCanvas
        }
        return renderCanvas
    }
}