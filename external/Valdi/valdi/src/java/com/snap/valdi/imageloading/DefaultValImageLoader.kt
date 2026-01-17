package com.snap.valdi.imageloading

import android.content.Context
import android.net.Uri
import android.util.Base64
import com.snap.valdi.bundle.LocalAssetLoader
import com.snap.valdi.exceptions.ValdiException
import com.snap.valdi.utils.ValdiAssetLoadOutputType
import com.snap.valdi.utils.ValdiImage
import com.snap.valdi.utils.ValdiImageContent
import com.snap.valdi.utils.ValdiImageFactory
import com.snap.valdi.utils.ValdiImageLoadCompletion
import com.snap.valdi.utils.ValdiImageLoadOptions
import com.snap.valdi.utils.ValdiImageLoader
import com.snap.valdi.utils.ValdiImageWithContent
import com.snap.valdi.utils.DelegatedLoader
import com.snap.valdi.utils.Disposable
import com.snap.valdi.utils.LoadCompletion
import com.snap.valdi.utils.LoaderDelegate
import com.snapchat.client.valdi_core.HTTPRequest
import com.snapchat.client.valdi_core.HTTPRequestManagerCompletion
import com.snapchat.client.valdi_core.HTTPResponse
import com.snapchat.client.valdi_core.HTTPRequestManager
import java.lang.ref.SoftReference

/**
 * An inefficient default image loader, not designed to use in production.
 */
class DefaultValdiImageLoader(val context: Context,
                                 val postprocessor: ValdiImageLoaderPostprocessor,
                                 val requestManager: HTTPRequestManager): ValdiImageLoader {

    data class CacheKey(val uri: Uri,
                        val outputType: ValdiAssetLoadOutputType
    )

    private val cache = hashMapOf<CacheKey, SoftReference<ValdiImage>>()

    private val loaderImpl = object: LoaderDelegate<CacheKey, ValdiImage> {
        override fun load(key: CacheKey, completion: LoadCompletion<ValdiImage>) {
            val cachedImage = getFromCache(key, 0, 0)
            if (cachedImage != null) {
                completion.onSuccess(cachedImage)
                return
            }

            val uri = key.uri

            if (LocalAssetLoader.isValdiAssetUrl(uri)) {
                val resId = LocalAssetLoader.resIDFromValdiAssetUrl(uri)
                loadImageResource(completion, key.outputType, resId)
            } else if (uri.scheme == "file") {
                loadImageFromFilePath(completion, key.outputType, uri.path ?: "")
            } else if (uri.scheme == "data") {
                loadImageFromDataScheme(completion, key.outputType, uri)
            } else {
                loadImageUri(completion, key.outputType, uri)
            }
        }
    }

    private val loader = DelegatedLoader(loaderImpl, postprocessor.executor)

    private fun createValdiImage(completion: LoadCompletion<ValdiImage>, createFunc: () -> ValdiImage) {
        try {
            val valdiImage = createFunc()
            completion.onSuccess(valdiImage)
        } catch (exception: Exception) {
            completion.onFailure(exception)
        }
    }

    private fun loadImageResource(completion: LoadCompletion<ValdiImage>, outputType: ValdiAssetLoadOutputType, resourceId: Int) {
        when (outputType) {
            ValdiAssetLoadOutputType.BITMAP -> {
                createValdiImage(completion) {
                    ValdiImageFactory.fromResources(context.resources, resourceId)
                }
            }
            ValdiAssetLoadOutputType.RAW_CONTENT -> {
                try {
                    val content = ValdiImageContent.fromStream(context.resources.openRawResource(resourceId))
                    completion.onSuccess(ValdiImageWithContent(content))
                } catch (exception: Exception) {
                    completion.onFailure(exception)
                }
            }
            else -> {
                // Something has gone horribly wrong
            }
        }
    }

    private fun loadImageFromData(completion: LoadCompletion<ValdiImage>, outputType: ValdiAssetLoadOutputType, body: ByteArray?) {
        if (body == null) {
            completion.onFailure(ValdiException("Did not receive response body"))
            return
        }

        when (outputType) {
            ValdiAssetLoadOutputType.BITMAP -> {
                createValdiImage(completion) {
                    ValdiImageFactory.fromByteArray(body)
                }
            }

            ValdiAssetLoadOutputType.RAW_CONTENT -> {
                completion.onSuccess(ValdiImageWithContent(ValdiImageContent.Bytes(body)))
            }
            else -> {
                // Something has gone horribly wrong
            }
        }
    }

    private fun loadImageFromFilePath(completion: LoadCompletion<ValdiImage>, outputType: ValdiAssetLoadOutputType, filePath: String) {
        when (outputType) {
            ValdiAssetLoadOutputType.BITMAP -> {
                createValdiImage(completion) {
                    ValdiImageFactory.fromFilePath(filePath)
                }
            }

            ValdiAssetLoadOutputType.RAW_CONTENT -> {
                completion.onSuccess(ValdiImageWithContent(ValdiImageContent.FileReference(filePath)))
            }
            else -> {
                // Something has gone horribly wrong
            }
        }
    }

    private fun loadImageFromDataScheme(completion: LoadCompletion<ValdiImage>, outputType: ValdiAssetLoadOutputType, uri: Uri) {
        val str = uri.toString()
        val delimiter = "base64,"
        val index = str.indexOf(delimiter)
        if (index < 0) {
            completion.onFailure(ValdiException("Invalid data URL, expecting base64"))
            return
        }
        
        val bytes = try {
            Base64.decode(str.substring(index + delimiter.length), Base64.DEFAULT)
        } catch (err: Throwable) {
            completion.onFailure(err)
            return
        }

        loadImageFromData(completion, outputType, bytes)
    }

    private fun loadImageUri(completion: LoadCompletion<ValdiImage>, outputType: ValdiAssetLoadOutputType, url: Uri) {
        requestManager.performRequest(HTTPRequest(url.toString(), "GET", null, null, 0), object: HTTPRequestManagerCompletion() {

            override fun onComplete(response: HTTPResponse) {
                loadImageFromData(completion, outputType, response.body)
            }

            override fun onFail(error: String) {
                completion.onFailure(ValdiException(error))
            }
        })
    }

    private fun getFromCache(url: CacheKey, requestedWidth: Int, requestedHeight: Int): ValdiImage? {
        return synchronized(cache) {
            cache[url]?.get()
        }
    }

    private fun storeInCache(url: CacheKey, requestedWidth: Int, requestedHeight: Int, image: ValdiImage) {
        val previousValue = synchronized(cache) {
            val previousValue = cache[url]?.get()
            cache[url] = SoftReference(image)

            previousValue
        }

        image.retain()
        previousValue?.release()
    }

    override fun getSupportedURLSchemes(): List<String> {
        return listOf("file", "http", "https", "data", LocalAssetLoader.VALDI_ASSET_SCHEME)
    }

    override fun getRequestPayload(url: Uri): Any {
        return url
    }

    private fun onImageLoaded(image: ValdiImage, options: ValdiImageLoadOptions, completion: ValdiImageLoadCompletion) {
        this.postprocessor.postprocess(image, options, completion)
    }

    override fun getSupportedOutputTypes(): Int {
        return ValdiAssetLoadOutputType.BITMAP.value or ValdiAssetLoadOutputType.RAW_CONTENT.value
    }

    override fun loadImage(requestPayload: Any, options: ValdiImageLoadOptions, completion: ValdiImageLoadCompletion): Disposable? {
        val url = requestPayload as Uri
        val key = CacheKey(url, options.outputType)
        val image = getFromCache(key, options.requestedWidth, options.requestedHeight)
        if (image != null) {
            onImageLoaded(image, options, completion)
            return null
        }

        return loader.load(key, object: LoadCompletion<ValdiImage> {

            override fun onSuccess(item: ValdiImage) {
                storeInCache(key, options.requestedWidth, options.requestedHeight, item)

                if (options.outputType == ValdiAssetLoadOutputType.BITMAP) {
                    onImageLoaded(item, options, completion)
                } else {
                    completion.onImageLoadComplete(0, 0, item, null)
                }
            }

            override fun onFailure(error: Throwable) {
                completion.onImageLoadComplete(0, 0, null, error)
            }
        })
    }
}
