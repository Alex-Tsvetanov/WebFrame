//
//  String+FileExtensions.swift
//  Compiler
//
//  Created by Brandon Francis on 8/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

extension String {

    func hasExtension(_ ext: String) -> Bool {
        return self.hasSuffix(".\(ext)")
    }

    func hasAnyExtension(_ extensions: [String]) -> Bool {
        return extensions.contains(where: { hasExtension($0) })
    }

    func deletingPathExtension() -> String {
        return (self as NSString).deletingPathExtension
    }

    func deletingLastPathComponent() -> String {
        return (self as NSString).deletingLastPathComponent
    }

    func lastPathComponent() -> String {
        return (self as NSString).lastPathComponent
    }

    static func sanitizePathComponents(_ components: String?...) -> String? {
        let sanitizedComponents =
            components
                .compactMap { $0 }
                .map { $0.split(separator: "/", maxSplits: Int.max, omittingEmptySubsequences: true) }
                .flatMap { $0 }

        if !sanitizedComponents.isEmpty {
            return sanitizedComponents.joined(separator: "/")
        } else {
            return nil
        }
    }

}
