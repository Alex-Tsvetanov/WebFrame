//
//  ObjCSelector.swift
//  Compiler
//
//  Created by Simon Corsin on 12/20/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct ObjcMessageParameter {
    let name: String
    let type: String
}

struct ObjCSelector {
    let messageParts: [String]
    let returnType: String
    let parameters: [ObjcMessageParameter]
    let messageDeclaration: String
    let selectorName: String

    init(returnType: String, methodName: String, parameters: [ObjcMessageParameter]) {
        self.returnType = returnType
        self.parameters = parameters

        if parameters.isEmpty {
            self.messageParts = [methodName]
            self.messageDeclaration = "- (\(returnType))\(methodName)"
            self.selectorName = methodName
        } else {
            var messageParts = [String]()
            var messageDeclaration = "- (\(returnType))"

            for (index, parameter) in parameters.enumerated() {
                let messagePart: String

                if index == 0 {
                    messagePart = "\(methodName)With\(parameter.name.pascalCased)"
                } else {
                    messagePart = parameter.name
                    messageDeclaration += " "
                }
                messageDeclaration += messagePart
                messageDeclaration += ":(\(parameter.type))\(parameter.name)"

                messageParts.append(messagePart)
            }

            self.messageParts = messageParts
            self.messageDeclaration = messageDeclaration
            self.selectorName = messageParts.map { "\($0):" }.joined()
        }
    }

    func makeCallString(parameterValues: [String]) throws -> String {
        if parameterValues.count != parameters.count {
            throw CompilerError("Invalid number of parameters, expected \(parameters.count), got \(parameterValues.count)")
        }

        if parameters.isEmpty {
            return messageParts[0]
        }

        return messageParts.enumerated().map {
                "\($0.element):\(parameterValues[$0.offset])"
        }.joined(separator: " ")
    }
}
