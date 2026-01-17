//
//  EmittedIdentifiers.swift
//  
//
//  Created by Simon Corsin on 2/17/23.
//

import Foundation

class EmittedIdentifiers {

    var isEmpty: Bool {
        return indexByIdentifier.isEmpty
    }

    private(set) var identifiers: [String] = []
    private var indexByIdentifier: [String: Int] = [:]

    func getIdentifierIndex(_ identifier: String) -> Int {
        if let index = self.indexByIdentifier[identifier] {
            return index
        }

        let index = identifiers.count
        indexByIdentifier[identifier] = index
        identifiers.append(identifier)
        return index
    }

}
