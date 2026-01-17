//
//  CompilationProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

protocol CompilationProcessor: CustomStringConvertible {

    func process(items: CompilationItems) throws -> CompilationItems

}
