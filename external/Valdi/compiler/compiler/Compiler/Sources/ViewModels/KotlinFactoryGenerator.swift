//
//  KotlinFactoryGenerator.swift
//
//
//  Created by Dorit Rein on 5/18/23.
//
import Foundation

private struct KotlinProperty {
    let propertyName: String
    let fieldName: String
    let modelProperty: ValdiModelProperty
}

final class KotlinFactoryGenerator {
    private let typeParameters: [ValdiTypeParameter]?
    private let properties: [ValdiModelProperty]
    private let writer: KotlinCodeGenerator
    private let jvmClass: JVMClass
    private let sourceFileName: GeneratedSourceFilename
    private let nameAllocator = PropertyNameAllocator.forKotlin()

    init(jvmClass: JVMClass, typeParameters: [ValdiTypeParameter]?, properties: [ValdiModelProperty], classMapping: ResolvedClassMapping, sourceFileName: GeneratedSourceFilename) {
        self.typeParameters = typeParameters
        self.properties = properties
        self.sourceFileName = sourceFileName

        self.jvmClass = jvmClass
        self.writer = KotlinCodeGenerator(package: jvmClass.package, classMapping: classMapping)
    }

    private struct ConstructorParameter {
        let typeParser: KotlinTypeParser
        let name: String
        let typeName: String
        let fieldName: String
        let parameterToField: String
    }

    private struct CreateMethodParameter {
        let typeParser: KotlinTypeParser
        let name: String
        let typeName: String
        let fieldName: String
        let injectable: Bool
        let isOptional: Bool
    }

    private func generateConstructor(parameters: [ConstructorParameter]) throws -> CodeWriter {
        let constructorBody = KotlinCodeGenerator()

        var constructorParameters = [String]()

        for parameter in parameters {
            let constructorParameter = "\(parameter.name): \(parameter.typeName)"
            constructorParameters.append(constructorParameter)
            constructorBody.appendBody(parameter.fieldName)
            constructorBody.appendBody(" = ")
            constructorBody.appendBody(parameter.parameterToField)
            constructorBody.appendBody("\n")
        }

        let constructor = KotlinCodeGenerator()
        let injectClass = try writer.importClass("javax.inject.Inject")

        constructor.appendBody("@\(injectClass.name) constructor(\(constructorParameters.joined(separator: ", "))) {\n")
        constructor.appendBody(constructorBody)
        constructor.appendBody("}\n\n")

        return constructor
    }

    private func generateCreateMethod(name: String,
                                      createMethodName: String,
                                      parameters: [CreateMethodParameter],
                                      indicesToOmit: Set<Int>) throws -> CodeWriter {
        var createMethodParameters = [String]()
        let createMethodBody = KotlinCodeGenerator()
        createMethodBody.appendBody("return \(name)(\n")

        for (index, parameter) in parameters.enumerated() {
            let isOmitted = indicesToOmit.contains(index)

            if !isOmitted && !parameter.injectable {
                createMethodParameters.append("\(parameter.name): \(parameter.typeName)")
            }

            createMethodBody.appendBody(parameter.name)

            if !isOmitted {
                createMethodBody.appendBody(" = \(parameter.name),\n")
            } else {
                createMethodBody.appendBody(" = null,\n")
            }
        }

        createMethodBody.appendBody(")\n")

        let createMethod = KotlinCodeGenerator()

        createMethod.appendBody("fun \(createMethodName)(\(createMethodParameters.joined(separator: ", "))): \(name) {\n")
        createMethod.appendBody(createMethodBody)
        createMethod.appendBody("}\n\n")

        return createMethod
    }

    private func resolveProperties(valdiProperties: [ValdiModelProperty]) -> [KotlinProperty] {
        return valdiProperties.map { valdiProperty in
            let propertyName = nameAllocator.allocate(property: valdiProperty.name).name
            let fieldName = nameAllocator.allocate(property: "_\(valdiProperty.name)").name

            return KotlinProperty(propertyName: propertyName,
                                  fieldName: fieldName,
                                  modelProperty: valdiProperty)
        }
    }

    private func writeConstructors(writer: KotlinCodeGenerator,
                                   constructors: CodeWriter,
                                   constructorParameters: [ConstructorParameter]) throws {
        let constructorAnnotation = try writer.getJavaAnnotationContent(annotationType: "com.snap.valdi.schema.ValdiClassConstructor")
        constructors.appendBody(constructorAnnotation)
        try constructors.appendBody(generateConstructor(parameters: constructorParameters))
    }

    private func writeCreateMethod(fieldsBody: CodeWriter,
                                   name: String,
                                   parameters: [CreateMethodParameter]) throws {
        try fieldsBody.appendBody(generateCreateMethod(name: name, createMethodName: "create", parameters: parameters, indicesToOmit: []))

        // Generate alternate create method parameters
        let alternateParameters = parameters.map { CreateMethodParameter(typeParser: $0.typeParser,
                                                                         name: $0.name,
                                                                         typeName: $0.typeName,
                                                                         fieldName: $0.fieldName,
                                                                         injectable: $0.injectable,
                                                                         isOptional: $0.isOptional) }

        var indicesToOmit: Set<Int> = []
        for index in 0..<alternateParameters.count {
            let resolvedNameAndType = alternateParameters[index]
            if resolvedNameAndType.isOptional && !resolvedNameAndType.injectable {
                indicesToOmit.insert(index)
            }
        }
        if !alternateParameters.isEmpty && indicesToOmit.count > 0 {
            let createMethod = try generateCreateMethod(name: name,
                                                        createMethodName: "createWithNullDefaults",
                                                        parameters: alternateParameters,
                                                        indicesToOmit: indicesToOmit)
            fieldsBody.appendBody("\n")
            fieldsBody.appendBody(createMethod)
        }
    }

    private func write(className: String, properties: [ValdiModelProperty], writer: KotlinCodeGenerator) throws {
        let marshallableObjectClass = try writer.importClass("com.snap.valdi.utils.ValdiMarshallableObject")

        let constructors = CodeWriter()
        let propertiesBody = CodeWriter()
        let fieldsBody = CodeWriter()

        writer.appendBody("\n")
        writer.appendBody(FileHeaderCommentGenerator.generateFactoryHeaderMessage(className: className,
                                                                                  sourceFilename: sourceFileName))
        writer.appendBody("\n")

        let descriptorAnnotation = try writer.getDescriptorAnnotation(type: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.valdiClass, typeParameters: typeParameters, additionalParameters: [])

        writer.appendBody(descriptorAnnotation)
        writer.appendBody("class \(className)Factory: \(marshallableObjectClass.name) {\n\n")
        writer.appendBody(propertiesBody)
        writer.appendBody("\n\n")
        writer.appendBody(fieldsBody)
        writer.appendBody("\n\n")
        writer.appendBody(constructors)
        writer.appendBody("}")

        // add all properties to constructor
        let kotlinProperties = resolveProperties(valdiProperties: properties)

        var resolvedNamesAndTypesForConstructor = [ConstructorParameter]()
        var resolvedNamesAndTypesForProperties = [CreateMethodParameter]()

        for (propertyIndex, kotlinProperty) in kotlinProperties.enumerated() {
            let property = kotlinProperty.modelProperty
            let isOptional = property.type.isOptional
            let propertyName = kotlinProperty.propertyName
            let typeParser = try writer.getTypeParser(type: property.type.unwrappingOptional,
                                                      isOptional: isOptional,
                                                      propertyName: kotlinProperty.modelProperty.name,
                                                      nameAllocator: nameAllocator.scoped())

            // only add injected properties to constructor
            if property.injectableParams.compatibility.contains(.android) {
                propertiesBody.appendBody("var \(propertyName): \(typeParser.fullTypeName)\n")
                propertiesBody.appendBody("    get() = \(kotlinProperty.fieldName)\n")
                propertiesBody.appendBody("    set(value) { \(kotlinProperty.fieldName) = value }\n\n")

                resolvedNamesAndTypesForConstructor.append(ConstructorParameter(
                    typeParser: typeParser,
                    name: property.name,
                    typeName: typeParser.fullTypeName,
                    fieldName: kotlinProperty.fieldName,
                    parameterToField: property.name))
            }

            let fieldAnnotation = try descriptorAnnotation.appendField(propertyIndex: propertyIndex, propertyName: property.name, kotlinFieldName: kotlinProperty.fieldName, expectedKotlinFieldName: "_\(property.name)", schemaBuilder: { schemaWriter in
                try schemaWriter.appendType(property.type, asBoxed: false, isMethod: false)
            })

            if property.injectableParams.compatibility.contains(.android) {
                fieldsBody.appendBody(fieldAnnotation)
                fieldsBody.appendBody("private var \(kotlinProperty.fieldName ): \(typeParser.fullTypeName)\n\n")
            }

            resolvedNamesAndTypesForProperties.append(CreateMethodParameter(
                typeParser: typeParser,
                name: property.name,
                typeName: typeParser.fullTypeName,
                fieldName: kotlinProperty.fieldName,
                injectable: property.injectableParams.compatibility.contains(.android),
                isOptional: property.type.isOptional))
        }

        try writeConstructors(writer: writer,
                              constructors: constructors,
                              constructorParameters: resolvedNamesAndTypesForConstructor)
        try writeCreateMethod(fieldsBody: fieldsBody,
                              name: className,
                              parameters: resolvedNamesAndTypesForProperties)
    }

    func write() throws -> File {
        try write(className: jvmClass.name, properties: properties, writer: writer)

        let data = try writer.content.indented(indentPattern: "    ").utf8Data()
        return .data(data)
    }
}
