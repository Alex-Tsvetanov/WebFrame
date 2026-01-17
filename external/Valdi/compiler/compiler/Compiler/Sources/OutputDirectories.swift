//
//  File.swift
//  
//
//  Created by Simon Corsin on 10/17/19.
//

import Foundation

struct OutDirectories {
    let baseURL: URL
    let assetsURL: URL
    /**
     Directory that will hold the platform specific generated sources, like Objective-C, Swift or Kotlin
     */
    let srcURL: URL

    /**
     Directory that will hold the C or C++ generated sources
     */
    let nativeSrcURL: URL

    let resourcesURL: URL // .bundle for iOS, res/ directory for Android
    let metadataURL: URL? // where to place module metadata (e.g. DI information)
}
