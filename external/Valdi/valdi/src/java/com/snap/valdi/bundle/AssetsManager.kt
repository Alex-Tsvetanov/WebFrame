package com.snap.valdi.bundle

import com.snapchat.client.valdi_core.Asset

interface AssetsManager {

    fun getUrlAsset(url: String): Asset

    fun getLocalAsset(moduleName: String, path: String): Asset
    
}