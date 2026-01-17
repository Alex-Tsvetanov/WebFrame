//
//  File.swift
//  
//
//  Created by Simon Corsin on 6/9/20.
//

import Foundation

class ObjCEmittedIdentifiers: EmittedIdentifiers {

    var initializationString: CodeWriter {
        let out = CodeWriter()
        out.appendBody("{\n")

        for identifier in identifiers {
            out.appendBody("\"\(identifier)\"")
            out.appendBody(",\n")
        }
        out.appendBody("nil\n")

        out.appendBody("}")
        return out
    }

}
