//
//  ObjCDependencyRepresentationGenerator.swift
//
//
//  Created by John Corbett on 5/16/23.
//

import Foundation

final class ObjCDependencyRepresentationGenerator {
    private struct ContextFactoryProperty: Encodable {
        let qualifiedType: String
        let name: String
        let propertyAttribute: String
        let injectable: Bool
    }

    private let items: [IntermediateDependencyMetadata]

    init(items: [IntermediateDependencyMetadata]) {
        self.items = items
    }

    func generate() throws -> File {
        let out = try items.reduce(into: [String: [ContextFactoryProperty]]()) { initialItems, dependencyMetadata in
            let model = dependencyMetadata.model

            guard let iosType = model.iosType else {
                return
            }

            let className = iosType.name
            let objcGenerator = ObjCClassGenerator(className: className)

            let nameAllocator = PropertyNameAllocator.forObjC()

            let properties = try model.properties.map { property in
                let typeParser = try objcGenerator.writeTypeParser(type: property.type.unwrappingOptional,
                                                                   isOptional: property.type.isOptional,
                                                                   namePaths: [property.name],
                                                                   allowValueType: true,
                                                                   isInterface: model.exportAsInterface,
                                                                   nameAllocator: nameAllocator.scoped())

                return ContextFactoryProperty(qualifiedType: typeParser.typeName,
                                              name: property.name,
                                              propertyAttribute: typeParser.propertyAttribute,
                                              injectable: property.injectableParams.compatibility.contains(.ios))
            }

            initialItems[className] = properties
        }

        let encoder = JSONEncoder()
        encoder.outputFormatting = .sortedKeys

        let data: Data
        do {
            data = try encoder.encode(out)
        } catch {
            throw CompilerError("Failed to encode dependency metadata")
        }

        return File.data(data)
    }
}
