//
//  URL+Symlink.swift
//  Compiler
//
//  Created by John Corbett on 9/25/24.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

import Foundation

enum SymlinkErrors: Error {
    case resolvedSymlinkDoesNotExist(String)
}

extension URL {
    func resolvingSymlink() throws -> URL {
        //  This code handles the resolution of symbolic links differently based on the operating system.
        //  On Linux, the native `resolvingSymlinksInPath` method is known to have bugs. Therefore, we leverages C APIs directly.
        //  https://github.com/swiftlang/swift-corelibs-foundation/issues/3685
        // 
        //  On non-Linux systems, the standard `resolvingSymlinksInPath` method is used to resolve the symbolic links.

        #if os(Linux)

        var buffer = [CChar](repeating: 0, count: Int(PATH_MAX))
        self.withUnsafeFileSystemRepresentation { fileSystemPath in
            if let fileSystemPath = fileSystemPath {
                _ = realpath(fileSystemPath, &buffer)
            }
        }
        var isDirectory: ObjCBool = false
        let resolvedPath = String(cString: buffer)
        if !FileManager.default.fileExists(atPath: resolvedPath, isDirectory: &isDirectory) {
            throw SymlinkErrors.resolvedSymlinkDoesNotExist(resolvedPath)
        }
        
        return URL(fileURLWithFileSystemRepresentation: buffer, isDirectory: isDirectory.boolValue, relativeTo: nil)

        #else  // os(MacOS)

        return self.resolvingSymlinksInPath()

        #endif  // end os(Linux)
    }

    func resolvingSymlink(resolveParentDirs: Bool) throws -> URL {
        if resolveParentDirs && !FileManager.default.fileExists(atPath: self.path) {
            var components = [String]()
            var current = self.standardized.deletingLastPathComponent()
            components.append(self.lastPathComponent)

            while true {
                if FileManager.default.fileExists(atPath: current.path) {
                    break
                }

                let component = current.lastPathComponent
                if component.isEmpty {
                    throw SymlinkErrors.resolvedSymlinkDoesNotExist(self.path)
                }

                components.append(component)
                current = current.deletingLastPathComponent()
            }

            let resolvedDir = try current.resolvingSymlink()

            return resolvedDir.appendingPathComponent(components.reversed().joined(separator: "/"))
        } else {
            return try resolvingSymlink()
        }
    }
}
