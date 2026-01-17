//
//  Constants.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//
//

import Foundation

let ValdiVersion: Int32 = 3

struct Ports {
    static let reloaderServiceDiscovery = 13253
    static let reloaderStandalone: Int = 13591
    static let reloaderOverUSB: Int = 13592
    static let valdiDebugger: Int = 13594
    static let hermesDebugger: Int = 13595
}

struct FileExtensions {

    static let valdiCss = "valdicss"
    static let css = "css"
    static let javascript = "js"
    static let kotlin = "kt"
    static let sass = "scss"
    static let svg = "svg"
    static let typescript = "ts"
    static let typescriptXml = "tsx"
    static let typescriptDeclaration = "d.ts"
    static let json = "json"
    static let bin = "bin"
    static let vue = "vue"
    static let zip = "zip"
    static let map = "map"
    static let protoDecl = "protodecl"
    static let sql = ["sq", "sqm"]
    static let valdiModule = "valdimodule"
    static let sourceMapJson = "map.json"
    static let assetCatalog = "assetcatalog"
    static let assetPackage = "assetpackage"
    static let downloadableArtifacts = "downloadableArtifacts"
    static let objc = ["h", "m", "mm"]
    static let swift = "swift"
    static let exportedImages = ["png", "webp", svg]
    static let images = FileExtensions.exportedImages + ["jpg", "jpeg"]
    static let legacyStyles = [FileExtensions.css, FileExtensions.sass]
    static let legacyModuleStyles = [FileExtensions.css, FileExtensions.sass].map { ".module.\($0)" }
    static let fonts = ["ttf"]

    static let typescriptFileExtensionsDotted = [FileExtensions.typescriptDeclaration, FileExtensions.typescript, FileExtensions.typescriptXml].map { ".\($0)" }

}

struct Files {
    static let classMapping = "class-mapping.xml"
    static let compilationMetadata = "compilation-metadata.json"
    static let downloadManifest = "download_manifest"
    static let hash = "hash"
    static let keepXML = "valdi_keep.xml"
    static let stringsJSONPrefix = "strings-"
    static let stringsJSONSuffix = ".json"
    static let stringsJSON = "\(stringsJSONPrefix)en\(stringsJSONSuffix)"
    static let ids = "ids.yaml"
    static let moduleYaml = "module.yaml"
    static let buildFile = "BUILD.bazel"
    static let sqlTypesYaml = "sql_types.yaml"
    static let sqlManifestYaml = "sql_manifest.yaml"
    static let sqlExportDir = "sqldelight"
}

struct Magic {
    static let valdiMagic: Data = {
        var packetData = Data()
        packetData.append(0x33)
        packetData.append(0xC6)
        packetData.append(0x00)
        packetData.append(0x01)
        return packetData
    }()
}

struct DeterministicDate {
    // a fixed date for deterministic builds: January 1, 2020 00:00:00 UTC
    static let fixedDate = Date(timeIntervalSince1970: 1577836800)
}
