//
//  URL+FileExtensions.swift
//  Compiler
//
//  Created by David Byttow on 10/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

extension URL {
    static var valdiDirectoryURL: URL {
        return URL(fileURLWithPath: NSHomeDirectory()).appendingPathComponent(".valdi", isDirectory: true)
    }

    static var logsDirectoryURL: URL {
        return valdiDirectoryURL.appendingPathComponent("logs", isDirectory: true)
    }

    static var deviceStorageDirectoryURL: URL {
        return valdiDirectoryURL.appendingPathComponent("device_storage", isDirectory: true)
    }

    static var valdiUserConfigURL: URL {
        return valdiDirectoryURL.appendingPathComponent("config.yaml", isDirectory: false)
    }

    func deletingPathExtensions(_ set: [String]) -> URL {
        let url = self.deletingLastPathComponent()
        let newLastPathComponent = self.lastPathComponent.removing(suffixes: set)
        return url.appendingPathComponent(newLastPathComponent)
    }

    var deletingTypeScriptFileExtensions: URL {
        return deletingPathExtensions(FileExtensions.typescriptFileExtensionsDotted)
    }
}
