package com.snap.valdi.utils

import com.snap.valdi.ValdiRuntime
import java.lang.RuntimeException
import java.lang.ref.WeakReference

object ValdiLeakTracker {

    var enabled = false

    private const val maxGCSurvival = 2

    private class TrackedRef(
            val ref: WeakReference<Any>,
            val name: String,
            val runtime: ValdiRuntime,
            var gcSurviveCounter: Int)

    private var trackedReferences = mutableListOf<TrackedRef>()
    private var thread: Thread? = null

    fun trackRef(ref: WeakReference<Any>, name: String, runtime: ValdiRuntime) {
        if (!enabled) {
            return
        }

        synchronized(this) {
            trackedReferences.add(TrackedRef(ref, name, runtime, 0))
            if (thread == null) {
                thread = Thread({
                    trackLeaks()
                }, "Valdi Leak Tracker")
                thread?.start()
            }
        }
    }

    fun untrackRef(ref: Any) {
        if (!enabled) {
            return
        }

        synchronized(this) {
            val it = trackedReferences.iterator()
            while (it.hasNext()) {
                val current = it.next()

                if (current.ref.get() === ref){
                    it.remove()
                }
            }
        }
    }

    private fun updateTrackedReferences(referencesToUntrack: MutableList<TrackedRef>, out: MutableList<TrackedRef>): Boolean {
        synchronized(this) {
            for (refToUntrack in referencesToUntrack) {
                trackedReferences.remove(refToUntrack)
            }
            referencesToUntrack.clear()

            out.clear()
            out.addAll(trackedReferences)

            if (out.isEmpty()) {
                thread = null
                return false
            }

            return true
        }
    }

    private fun trackLeaks() {
        val referencesToTrack = mutableListOf<TrackedRef>()
        val referencesToUntrack = mutableListOf<TrackedRef>()

        while (true) {
            Thread.sleep(5000)

            if (!updateTrackedReferences(referencesToUntrack, referencesToTrack)) {
                return
            }

            System.gc()

            val runtimes = referencesToTrack.groupBy { it.runtime }
            for (entry in runtimes) {
                val runtime = entry.key

                for (trackedRef in entry.value) {
                    val trackedInstance = trackedRef.ref.get()
                    if (trackedInstance != null) {
                        runtime.logger.debug("${trackedRef.name} is still alive")
                        trackedRef.gcSurviveCounter += 1

                        if (trackedRef.gcSurviveCounter >= maxGCSurvival) {
                            throw RuntimeException("Valdi reference ${trackedRef.name} is unexpectedly still alive after ${trackedRef.gcSurviveCounter} GCs. Did you call destroy() on the root view?")
                        }
                    } else {
                        referencesToUntrack.add(trackedRef)
                    }
                }

                runtime.performGcNow(true)
            }
        }
    }

}
