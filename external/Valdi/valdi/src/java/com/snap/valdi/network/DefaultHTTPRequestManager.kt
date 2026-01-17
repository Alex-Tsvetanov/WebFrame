package com.snap.valdi.network

import android.content.Context
import com.snap.valdi.utils.ExecutorsUtil
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors
import com.snapchat.client.valdi.*
import com.snapchat.client.valdi_core.*

class DefaultHTTPRequestManager(context: Context): HTTPRequestManager() {

    private class RequestTask(val url: URL, val method: String, val body: ByteArray?, val headers: Map<String, String>, completion: HTTPRequestManagerCompletion): HTTPRequestTask(completion), Runnable {

        private fun doPerformRequestWithURLConnection(urlConnection: HttpURLConnection): HTTPResponse {
            urlConnection.instanceFollowRedirects = true

            try {
                urlConnection.requestMethod = method

                for (entry in headers.entries) {
                    urlConnection.setRequestProperty(entry.key, entry.value)
                }

                urlConnection.doInput = true

                if (body != null) {
                    urlConnection.doOutput = true
                    urlConnection.outputStream.write(body)
                    urlConnection.outputStream.close()
                }

                val responseCode = urlConnection.responseCode

                val responseHeaders = hashMapOf<String, String>()
                for (responseHeader in urlConnection.headerFields) {
                    if (!responseHeader.key.isNullOrEmpty() && responseHeader.value.isNotEmpty()) {
                        responseHeaders[responseHeader.key] = responseHeader.value.first()
                    }
                }

                val stream: InputStream? = if (responseCode >= 300) urlConnection.errorStream else urlConnection.inputStream

                val bodyResponse = stream?.readBytes()

                return HTTPResponse(responseCode, responseHeaders, bodyResponse)
            } finally {
                try {
                    urlConnection.disconnect()
                } catch (exc: Exception) {}
            }
        }

        private fun performRequest(): HTTPResponse {
            val urlConnection = url.openConnection()

            if (urlConnection is HttpURLConnection) {
                return doPerformRequestWithURLConnection(urlConnection)
            } else {
                urlConnection.doInput = true
                val bodyResponse = urlConnection.getInputStream().readBytes()

                return HTTPResponse(200, hashMapOf<String, String>(), bodyResponse)
            }
        }

        override fun run() {
            try {
                val response = performRequest()
                notifySuccess(response)
            } catch (error: Exception) {
                notifyFailure("HTTP Request failed: ${error.message}")
            }
        }

        companion object {

            fun from(request: HTTPRequest, completion: HTTPRequestManagerCompletion): RequestTask {
                val url = URL(request.url)
                val method = request.method
                val body = request.body
                val headers = hashMapOf<String, String>()

                val headersMap = request.headers as? Map<*, *>
                if (headersMap != null) {
                    for (entry in headersMap.entries) {
                        val headerName = entry.key as? String
                        val headerValue = entry.value as? String
                        if (headerName != null && headerValue != null) {
                            headers[headerName] = headerValue
                        }
                    }
                }

                return RequestTask(url, method, body, headers, completion)
            }
        }
    }

    private var executors = ExecutorsUtil.newSingleThreadCachedExecutor { r ->
        Thread(r).apply {
            name = "Valdi Network Thread"
            priority = Thread.NORM_PRIORITY
        }
    }

    override fun performRequest(request: HTTPRequest, completion: HTTPRequestManagerCompletion): Cancelable {
        val task: RequestTask
        try {
            task = RequestTask.from(request, completion)
        } catch (exception: Exception) {
            completion.onFail("Failed to build request: ${exception.message}")

            return object: Cancelable() {
                override fun cancel() {
                }
            }
        }

        executors.submit(task)

        return task
    }

}
