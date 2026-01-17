//
//  File.swift
//  
//
//  Created by Simon Corsin on 2/17/23.
//

import Foundation

class KotlinEmittedTypeReferences: EmittedIdentifiers, CodeWriterContent {

    var initializationString: CodeWriter {
        let body = identifiers.map {
             "\($0)::class"
        }.joined(separator: ", ")
        let out = CodeWriter()
        out.appendBody("[")
        out.appendBody(body)
        out.appendBody("]")
        return out
    }

    var content: String {
        return initializationString.content
    }

}
