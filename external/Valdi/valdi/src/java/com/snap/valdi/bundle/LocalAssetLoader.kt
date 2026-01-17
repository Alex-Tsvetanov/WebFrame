package com.snap.valdi.bundle

import android.content.Context
import android.net.Uri
import com.snap.valdi.exceptions.ValdiException

class LocalAssetLoader(val context: Context) {

    companion object {
        const val VALDI_ASSET_SCHEME = "valdi-res"

        fun isValdiAssetUrl(url: Uri): Boolean {
            return url.scheme == VALDI_ASSET_SCHEME
        }

        fun valdiAssetUrlFromResID(resId: Int): Uri {
            return Uri.parse("${VALDI_ASSET_SCHEME}://${resId}")
        }

        fun resIDFromValdiAssetUrl(url: Uri): Int {
            if (!isValdiAssetUrl(url)) {
                throw ValdiException("'${url}' is not a ValdiAsset URL")
            }

            return url.host!!.toInt()
        }
    }
}
