//
//  KotlinSchemaWriter.swift
//
//
//  Created by Simon Corsin on 1/26/23.
//

import Foundation

class KotlinSchemaWriter: SchemaWriter {
    init(typeParameters: [ValdiTypeParameter]?, listener: SchemaWriterListener) {
        super.init(typeParameters: typeParameters,
                   listener: listener,
                   alwaysBoxFunctionParametersAndReturnValue: true,
                   boxIntEnums: false)
    }
}
