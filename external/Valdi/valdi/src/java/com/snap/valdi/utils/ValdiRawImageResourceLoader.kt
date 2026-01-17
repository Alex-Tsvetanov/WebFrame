package com.snap.valdi.utils

import android.content.Context
import android.net.Uri
import android.util.TypedValue
import com.snap.valdi.bundle.LocalAssetLoader
import java.util.concurrent.Executor

/**
 * ValdiImageLoader implementation that can load local assets into bytes assets.
 */
class ValdiRawImageResourceLoader(val executor: Lazy<Executor>, val context: Context): ValdiImageLoader {

    override fun getSupportedURLSchemes(): List<String> {
        return listOf(LocalAssetLoader.VALDI_ASSET_SCHEME)
    }

    override fun getSupportedOutputTypes(): Int {
        return ValdiAssetLoadOutputType.RAW_CONTENT.value
    }

    override fun getRequestPayload(url: Uri): Any {
        return LocalAssetLoader.resIDFromValdiAssetUrl(url)
    }

    private fun doLoadImage(resId: Int, completion: ValdiImageLoadCompletion) {
        val inputStream = try {
            val value = TypedValue()
            context.resources.openRawResource(resId, value)
        } catch (exc: Exception) {
            // Catching raw Exception to work around some Resources implementation issues
            // on Redmi and Xiaomi devices. They sometimes throw a NullPointerException
            completion.onImageLoadComplete(0, 0, null, exc)
            return
        }

        val imageContent = ValdiImageContent.fromStream(inputStream)

        completion.onImageLoadComplete(0, 0, ValdiImageWithContent(imageContent), null)
    }

    override fun loadImage(requestPayload: Any, options: ValdiImageLoadOptions, completion: ValdiImageLoadCompletion): Disposable? {
        val resId = requestPayload as Int

        val runnable = DisposableRunnable {
            doLoadImage(resId, completion)
        }

        executor.value.execute(runnable)

        return runnable
    }
}