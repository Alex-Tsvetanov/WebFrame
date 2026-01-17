package com.snap.valdi.utils

import java.lang.reflect.Method
import java.util.concurrent.atomic.AtomicInteger
import android.os.Trace

var VALDI_TRACER: Tracer? = null
// Max name length for atrace
private const val MAX_ATRACE_LEN = 127

private const val TRACE_TAG_APP: Long = 1L shl 12

private val currentTracingRecordCookie = AtomicInteger(0)

interface Tracer {
    fun beginSection(name: String)
    fun endSection()
    fun traceCounter(tag: String, value: Int)
    fun asyncTraceBegin(tag: String): Int
    fun asyncTraceEnd(tag: String, cookieId: Int)
}

class OSTracer: Tracer {

    private val setAppTracingAllowed by lazy {
        getTraceMethod("setAppTracingAllowed", Boolean::class.java)
    }

    private fun updateAppTracingAllowed() {
        // On Android 11 and below, passing the app package name to atrace is not enough to enable app-level atrace spans.
        // We manually manage whether app-level tracing is allowed to ensure our custom app spans can appear in systrace/
        // perfetto traces.

        try {
            setAppTracingAllowed?.invoke(null, true)
        } catch (ignored: Throwable) {
        }
    }

    // This init block MUST come AFTER the setAppTracingAllowed property declaration above.
    init {
        updateAppTracingAllowed()
    }

    private val asyncTraceBeginMethod by lazy {
        getTraceMethod("asyncTraceBegin", Long::class.java, String::class.java, Integer.TYPE)
    }

    private val asyncTraceEndMethod by lazy {
        getTraceMethod("asyncTraceEnd", Long::class.java, String::class.java, Integer.TYPE)
    }

    private val traceCounterMethod by lazy {
        getTraceMethod("traceCounter", Long::class.java, String::class.java, Integer.TYPE)
    }

    override fun asyncTraceBegin(tag: String): Int {
        val cookie = currentTracingRecordCookie.getAndIncrement()
        asyncTraceBeginMethod?.invoke(null, TRACE_TAG_APP, trimName(tag), cookie)
        return cookie
    }
    override fun asyncTraceEnd(tag: String, cookieId: Int) {
        asyncTraceEndMethod?.invoke(null, TRACE_TAG_APP, trimName(tag), cookieId)
    }

    override fun traceCounter(tag: String, value: Int) {
        traceCounterMethod?.invoke(null, TRACE_TAG_APP, trimName(tag), value)
    }

    override fun beginSection(name: String) {
        Trace.beginSection(trimName(name))
    }

    override fun endSection() {
        Trace.endSection()
    }

    private fun getTraceMethod(name: String, vararg parameterTypes: Class<*>): Method? {
        return try {
            Trace::class.java.getMethod(name, *parameterTypes)
        } catch (ignored: Throwable) {
            null
        }
    }

    private fun trimName(name: String) = if (name.length > MAX_ATRACE_LEN) {
        name.substring(0, MAX_ATRACE_LEN)
    } else {
        name
    }
}

inline fun <T> trace(name: () -> String, cb: () -> T): T {
    val tracer = VALDI_TRACER
    tracer?.beginSection(name())
    try {
        return cb()
    } finally {
        tracer?.endSection()
    }
}

data class AsyncSpan (
    val name: String,
    val cookie: Int,
)

inline fun asyncTraceBegin(name: String): AsyncSpan? {
    val tracer = VALDI_TRACER
    if (tracer == null) {
        return null
    }
    return AsyncSpan(name = name, cookie = tracer.asyncTraceBegin(name))
}

inline fun asyncTraceEnd(span: AsyncSpan?) {
    if (span != null) {
        val tracer = VALDI_TRACER
        tracer?.asyncTraceEnd(span.name, span.cookie)
    }
}
