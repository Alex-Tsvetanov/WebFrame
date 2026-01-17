//
//  TypeScriptSymbolParser.swift
//  Compiler
//
//  Created by Simon Corsin on 12/18/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

final class TypeScriptSymbolParser {

    private(set) var currentLocation: Int = 0

    private let symbols: [TS.SymbolDisplayPart]

    init(symbols: [TS.SymbolDisplayPart]) {
        self.symbols = symbols
    }

}
