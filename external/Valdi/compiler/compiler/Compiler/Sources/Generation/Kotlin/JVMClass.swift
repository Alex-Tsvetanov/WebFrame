//
//  JVMClass.swift
//  Compiler
//
//  Created by Simon Corsin on 8/13/19.
//

import Foundation

struct JVMClass {

    let fullClassName: String
    let name: String
    let package: String?

    var filePath: String {
        var components = fullClassName.split(separator: ".")
        if components.count > 0 {
            components.removeLast()
        }

        return components.joined(separator: "/")
    }

    init(fullClassName: String) {
        self.fullClassName = fullClassName

        let components = fullClassName.split(separator: ".")
        if components.count > 1 {
            name = String(components.last!)
            package = components[0..<components.count - 1].joined(separator: ".")
        } else {
            name = fullClassName
            package = nil
        }
    }
}
