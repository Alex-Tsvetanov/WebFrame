//
//  LanguageSpecificViewClassGenerator.swift
//  Compiler
//
//  Created by Simon Corsin on 5/17/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

protocol LanguageSpecificViewClassGenerator {
    static var platform: Platform { get }

    func start() throws
    func finish()
    func write() throws -> [NativeSource]

    func appendAccessor(className: String, nodeId: String) throws
    func appendConstructor() throws
//    func appendViewModelAccessor(viewModelClassName: String) throws
    func appendNativeActions(nativeFunctionNames: [String])
    func appendEmitActions(actions: [String])

}
