// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation
import Yams

/**
 Holds a set of include or exclude patterns that helps resolving the inclusion or exclusion of an item for a rule.
 */
struct InclusionConfig {
    enum InclusionResult {
        /**
         The item matches one of the include patterns
         */
        case included
        /**
         The item matches one of the exclude patterns
         */
        case excluded
        /**
         The item did match any of the patterns.
         */
        case unknown
    }

    let includePatterns: [NSRegularExpression]
    let excludePatterns: [NSRegularExpression]

    static let alwaysIncluded = InclusionConfig(includePatterns: [try! NSRegularExpression(pattern: ".*")], excludePatterns: [])

    private static func parsePatterns(from nodes: [Yams.Node]) throws -> [NSRegularExpression] {
        return try nodes.compactMap { $0.string }.map { try NSRegularExpression(pattern: $0, options: []) }
    }

    static func parse(from mapping: Yams.Node) throws -> InclusionConfig {
        let includePatterns = try parsePatterns(from: mapping["include_patterns"]?.array() ?? [])
        let excludePatterns = try parsePatterns(from: mapping["exclude_patterns"]?.array() ?? [])

        return InclusionConfig(includePatterns: includePatterns, excludePatterns: excludePatterns)
    }

    /**
     Resolves whether the given item matches one of the include or exclude patterns.
     */
    func resolve(_ str: String) -> InclusionResult {
        if isExcluded(str) {
            return .excluded
        }

        if isIncluded(str) {
            return .included
        }

        return .unknown
    }

    private func isIncluded(_ str: String) -> Bool {
        for pattern in includePatterns {
            if str.matches(regex: pattern) {
                return true
            }
        }

        return false
    }

    private func isExcluded(_ str: String) -> Bool {
        for pattern in excludePatterns {
            if str.matches(regex: pattern) {
                return true
            }
        }

        return false
    }
}
