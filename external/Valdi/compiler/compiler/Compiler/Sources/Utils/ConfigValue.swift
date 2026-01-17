//
//  File.swift
//  
//
//  Created by Simon Corsin on 10/18/21.
//

import Foundation
import Yams

struct ConfigValue {

    let keyPath: [String]
    let value: String

    static func parse(text: String) throws -> ConfigValue {
        guard let index = text.firstIndex(of: "=") else {
            throw CompilerError("Missing =")
        }

        let keyPathStr = text[text.startIndex..<index].trimmed.split(separator: ".")
        let value = text[text.index(after: index)...].trimmed

        return ConfigValue(keyPath: keyPathStr.map { String($0) }, value: String(value))
    }

    func inject(into node: Yams.Node) -> Yams.Node {
        return onInject(index: 0, node: node)
    }

    private func onInject(index: Int, node: Yams.Node?) -> Yams.Node {
        if index == keyPath.count {
            return Yams.Node(value)
        } else {
            let key = keyPath[index]

            let updatedNode = onInject(index: index + 1, node: node?[key])

            var out = node ?? Yams.Node.mapping(Yams.Node.Mapping([]))
            out[key] = updatedNode

            return out
        }
    }

}
