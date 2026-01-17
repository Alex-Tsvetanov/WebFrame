package com.snap.valdi.imageloading

import android.content.Context
import android.graphics.Bitmap
import com.snap.valdi.drawables.utils.BlurUtils
import com.snap.valdi.utils.BitmapPool
import com.snap.valdi.utils.ValdiAssetLoadOutputType
import com.snap.valdi.utils.ValdiImage
import com.snap.valdi.utils.ValdiImageContent
import com.snap.valdi.utils.ValdiImageLoadCompletion
import com.snap.valdi.utils.ValdiImageLoadOptions
import com.snap.valdi.utils.ExecutorsUtil

class ValdiImageLoaderPostprocessor(val context: Context, val bitmapPool: BitmapPool) {
   val executor = ExecutorsUtil.newSingleThreadCachedExecutor()

    private fun makeConsumerImage(image: ValdiImage): ValdiImage {
        image.retain()

        return object: ValdiImage() {
            override fun onDestroyBitmap() {
                image.release()
            }

            override fun getBitmap(): Bitmap {
                return image.getBitmap()
            }

            override fun getContent(): ValdiImageContent {
                return image.getContent()
            }
        }
    }

    private fun blurImage(image: ValdiImage, blurRadius: Float): ValdiImage {
        val bitmap = image.getContentAsBitmap() ?: return image
        val blurredImage = BlurUtils.blur(context, bitmapPool, bitmap, blurRadius) ?: return image

        return object: ValdiImage() {
            override fun onDestroyBitmap() {
                blurredImage.release()
            }

            override fun getContent(): ValdiImageContent {
                return ValdiImageContent.Bitmap(blurredImage.getBitmap())
            }

            override fun getBitmap(): Bitmap {
                return blurredImage.getBitmap()
            }
        }
    }

    fun postprocess(image: ValdiImage,
                    options: ValdiImageLoadOptions,
                    completion: ValdiImageLoadCompletion) {
        var imageWidth = 0
        var imageHeight = 0

        if (options.outputType == ValdiAssetLoadOutputType.BITMAP) {
            val bitmap = image.getBitmap()
            imageWidth = bitmap.width
            imageHeight = bitmap.height
        }

        if (options.blurRadius == 0.0f || options.outputType == ValdiAssetLoadOutputType.RAW_CONTENT) {
            completion.onImageLoadComplete(imageWidth, imageHeight, makeConsumerImage(image), null)
        } else {
            image.retain()
            executor.execute {
                try {
                    val blurredImage = blurImage(image, options.blurRadius)
                    completion.onImageLoadComplete(imageWidth, imageHeight, blurredImage, null)
                } catch (exc: Exception) {
                    completion.onImageLoadComplete(0, 0, null, exc)
                } finally {
                    image.release()
                }
            }
        }
    }
}