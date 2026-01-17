package com.snap.valdi.utils

import android.graphics.Path
import android.graphics.PathMeasure

class PathInterpolator {

    val empty: Boolean
        get() = pathMeasures.isEmpty()

    private val pathMeasures = arrayListOf<PathMeasure>()
    private var totalLength = 0.0f
    private val outputPath = Path()

    fun setPath(path: Path) {
        reset()

        // Android does not expose the PathContour API, so there is no way to efficiently
        // index all the contours in one iteration. We thus create one PathMeasure to iterate over
        // the contours, we then generate a Path representing the entire contour, on which we
        // create a PathMeasure just for that contour, so that we can later iterate on it.
        val pathMeasure = PathMeasure(path, false)

        do {
            val contourLength = pathMeasure.length

            val contourPath = Path()
            pathMeasure.getSegment(0.0f, contourLength, contourPath, true)

            pathMeasures.add(PathMeasure(contourPath, false))

            this.totalLength += contourLength
        } while (pathMeasure.nextContour())
    }

    fun reset() {
        totalLength = 0.0f
        pathMeasures.clear()
    }

    fun interpolate(start: Float, end: Float): Path {
        outputPath.reset()

        val absoluteStart = start * totalLength
        val absoluteEnd = end * totalLength

        var currentStart = 0.0f
        for (pathMeasure in pathMeasures) {
            val currentLength = pathMeasure.length
            val currentEnd = currentStart + currentLength

            if (currentStart >= absoluteEnd) {
                break
            }

            val relativeStart = Math.min(Math.max(absoluteStart - currentStart, 0.0f), currentLength)
            val relativeEnd = Math.min(absoluteEnd - currentStart, currentLength)

            if (relativeStart != relativeEnd) {
                if (!pathMeasure.getSegment(relativeStart, relativeEnd, outputPath, true)) {
                    break
                }
            }

            currentStart = currentEnd;
        }
        return outputPath
    }

}