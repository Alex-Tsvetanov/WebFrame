package com.snap.valdi.snapdrawing

import android.graphics.Path
import com.snap.valdi.exceptions.ValdiFatalException

object PathUtils {

    const val COMPONENT_MOVE = 1f
    const val COMPONENT_LINE = 2f
    const val COMPONENT_QUAD = 3f
    const val COMPONENT_CUBIC = 4f
    const val COMPONENT_ROUND_RECT = 5f
    const val COMPONENT_ARC = 6f
    const val COMPONENT_CLOSE = 7f

    fun parsePath(pathComponents: FloatArray, outPath: Path) {
        var index = 0
        while (index < pathComponents.size) {
            val component = pathComponents[index]
            index++
            when (component) {
                COMPONENT_MOVE -> {
                    outPath.moveTo(pathComponents[index++], pathComponents[index++])
                }
                COMPONENT_LINE -> {
                    outPath.lineTo(pathComponents[index++], pathComponents[index++])
                }
                COMPONENT_QUAD -> {
                    outPath.quadTo(
                            pathComponents[index++],
                            pathComponents[index++],
                            pathComponents[index++],
                            pathComponents[index++]
                    )
                }
                COMPONENT_CUBIC -> {
                    outPath.cubicTo(
                            pathComponents[index++],
                            pathComponents[index++],
                            pathComponents[index++],
                            pathComponents[index++],
                            pathComponents[index++],
                            pathComponents[index++]
                    )
                }
                COMPONENT_CLOSE -> {
                    outPath.close()
                }
                else -> throw ValdiFatalException("Invalid Path component $component")
            }

        }
    }

}