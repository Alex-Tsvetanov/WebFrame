//
//  KotlinViewModelGenerator.swift
//  Compiler
//
//  Created by Simon Corsin on 5/17/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

private struct KotlinProperty {
    let propertyName: String
    let fieldName: String
    let getterName: String?
    let modelProperty: ValdiModelProperty
}

private struct KotlinGenericTypeInfo {
    let typeName: String
}

private struct KotlinTypeParameter {
    let typeParameterName: String
    let valdiTypeParameter: ValdiTypeParameter
}

final class KotlinModelGenerator {
    private let bundleInfo: CompilationItem.BundleInfo
    private let typeParameters: [ValdiTypeParameter]?
    private let properties: [ValdiModelProperty]
    private let classMapping: ResolvedClassMapping
    private let writer: KotlinCodeGenerator
    private let jvmClass: JVMClass
    private let sourceFileName: GeneratedSourceFilename
    private let isInterface: Bool
    private let emitLegacyConstructors: Bool
    private let comments: String?
    private let usePublicFields: Bool
    private let nameAllocator = PropertyNameAllocator.forKotlin()

    init(bundleInfo: CompilationItem.BundleInfo,
         className: String,
         typeParameters: [ValdiTypeParameter]?,
         properties: [ValdiModelProperty],
         classMapping: ResolvedClassMapping,
         sourceFileName: GeneratedSourceFilename,
         isInterface: Bool,
         emitLegacyConstructors: Bool,
         comments: String?,
         usePublicFields: Bool = false) {
        
        self.bundleInfo = bundleInfo
        self.typeParameters = typeParameters
        self.properties = properties
        self.classMapping = classMapping
        self.sourceFileName = sourceFileName
        self.isInterface = isInterface
        self.emitLegacyConstructors = emitLegacyConstructors
        self.comments = comments
        self.usePublicFields = usePublicFields

        jvmClass = JVMClass(fullClassName: className)
        self.writer = KotlinCodeGenerator(package: jvmClass.package, classMapping: classMapping)
    }

    private struct ConstructorParameter {
        let typeParser: KotlinTypeParser
        let name: String
        let typeName: String
        let fieldName: String
        let parameterToField: String
        let isFunction: Bool
        let isOptional: Bool
        let constructorOmitted: Bool
    }

    private func generateConstructor(parameters: [ConstructorParameter],
                                     indexesToOmit: Set<Int>) -> CodeWriter {
                                        
        let constructorBody = KotlinCodeGenerator()

        var constructorParameters = [String]()

        for (index, parameter) in parameters.enumerated() {
            let isOmitted = indexesToOmit.contains(index)

            if !isOmitted {
                let constructorParameter = "\(parameter.name): \(parameter.typeName)\(parameter.isOptional ? " = null" : "")"
                constructorParameters.append(constructorParameter)
                constructorBody.appendBody("this." + parameter.fieldName)
                constructorBody.appendBody(" = ")
                constructorBody.appendBody(parameter.parameterToField)
                constructorBody.appendBody("\n")
            } else {
                constructorBody.appendBody("this." + parameter.fieldName)
                constructorBody.appendBody(" = null\n")
            }
        }

        let constructor = KotlinCodeGenerator()
        constructor.appendBody("constructor(\(constructorParameters.joined(separator: ", "))) {\n")
        constructor.appendBody(constructorBody)
        constructor.appendBody("}\n\n")

        return constructor
    }

    private func resolveTypeParameters(typeParameters: [ValdiTypeParameter]?) -> [KotlinTypeParameter]? {
        guard let typeParameters = typeParameters else { return nil }

        return typeParameters.map {
            let allocatedName = nameAllocator.allocate(property: $0.name).name
            return KotlinTypeParameter(typeParameterName: allocatedName, valdiTypeParameter: $0)
        }
    }

    private func resolvePropertyGetterName(propertyName: String) -> String {
        if propertyName.hasPrefix("is") && propertyName[propertyName.index(propertyName.startIndex, offsetBy: 2)].isUppercase {
            return propertyName
        } else {
            return"get\(propertyName.pascalCased)"
        }
    }

    private func resolveProperties(valdiProperties: [ValdiModelProperty]) -> [KotlinProperty] {
        return valdiProperties.map {
            let propertyName = nameAllocator.allocate(property: $0.name).name
            let fieldName = nameAllocator.allocate(property: "_\($0.name)").name
            let getterFieldName: String?

            if isInterface && !$0.type.unwrappingOptional.isFunction {
                let resolvedGetterFieldName = resolvePropertyGetterName(propertyName: $0.name)
                if resolvedGetterFieldName != $0.name {
                    getterFieldName = nameAllocator.allocate(property: resolvedGetterFieldName).name
                } else {
                    getterFieldName = resolvedGetterFieldName
                }
            } else {
                getterFieldName = nil
            }

            return KotlinProperty(propertyName: propertyName,
                                  fieldName: fieldName,
                                  getterName: getterFieldName,
                                  modelProperty: $0)
        }
    }

    private func appendNextOmittedParameter(constructors: [ConstructorParameter], indexesToOmit: inout Set<Int>) -> Bool {
        for index in (0..<constructors.count).reversed() {
            let constructor = constructors[index]
            guard constructor.constructorOmitted else { continue }

            if !indexesToOmit.contains(index) {
                indexesToOmit.insert(index)
                return true
            }
        }

        return false
    }

    private func shouldGenerateAlternateMainConstructor(constructorParameters: [ConstructorParameter], alternateConstructorParameters: [ConstructorParameter]) -> Bool {
        for index in 0..<constructorParameters.count {
            if constructorParameters[index].typeName != alternateConstructorParameters[index].typeName {
                return true
            }
        }

        return false
    }

    private func writeConstructors(writer: KotlinCodeGenerator,
                                   constructors: CodeWriter,
                                   constructorParameters: [ConstructorParameter]) throws {
        // Check if number of parameters exceeds Java's 255 parameter limit
        if constructorParameters.count > 250 {
            throw CompilerError("Too many constructor parameters: \(constructorParameters.count). Maximum allowed is 250 parameters to avoid Java bytecode limitations.")
        }
        
        // Generate main constructor
        let constructorAnnotation = try writer.getJavaAnnotationContent(annotationType: "com.snap.valdi.schema.ValdiClassConstructor")
        constructors.appendBody(constructorAnnotation)
        constructors.appendBody(generateConstructor(parameters: constructorParameters, indexesToOmit: []))

        // Generate alternate constructor parameters
        let alternateConstructorParameters = constructorParameters.map { ConstructorParameter(typeParser: $0.typeParser,
                                                                                              name: $0.name,
                                                                                              typeName: $0.typeParser.fullTypeName,
                                                                                              fieldName: $0.fieldName,
                                                                                              parameterToField: $0.name,
                                                                                              isFunction: $0.isFunction,
                                                                                              isOptional: $0.isOptional,
                                                                                              constructorOmitted: $0.constructorOmitted) }

        if emitLegacyConstructors {
            // Generate constructors using the legacy technique
            var indexesToOmit: Set<Int> = []

            if !constructorParameters.isEmpty {
                if shouldGenerateAlternateMainConstructor(constructorParameters: constructorParameters, alternateConstructorParameters: alternateConstructorParameters) {
                    let mainConstructor = generateConstructor(parameters: alternateConstructorParameters, indexesToOmit: indexesToOmit)
                    constructors.appendBody("\n")
                    constructors.appendBody(mainConstructor)
                }

                while appendNextOmittedParameter(constructors: alternateConstructorParameters, indexesToOmit: &indexesToOmit) {
                    let alternateConstructor = generateConstructor(parameters: alternateConstructorParameters, indexesToOmit: indexesToOmit)
                    constructors.appendBody("\n\n")
                    constructors.appendBody(alternateConstructor)
                }
            }
        } else {
            // Generate constructors using the new recommended minimal constructors technique
            var indexesToOmit: Set<Int> = []
            for index in 0..<alternateConstructorParameters.count {
                let resolvedNameAndType = alternateConstructorParameters[index]
                if resolvedNameAndType.isOptional {
                    indexesToOmit.insert(index)
                }
            }
            if !alternateConstructorParameters.isEmpty {
                if indexesToOmit.count > 0 || shouldGenerateAlternateMainConstructor(constructorParameters: constructorParameters, alternateConstructorParameters: alternateConstructorParameters) {
                    let mainConstructor = generateConstructor(parameters: alternateConstructorParameters, indexesToOmit: indexesToOmit)
                    constructors.appendBody("\n")
                    constructors.appendBody(mainConstructor)
                }
            }
        }
    }

    private func write(className: String, properties: [ValdiModelProperty], writer: KotlinCodeGenerator, usePublicFields: Bool) throws {
        let marshallableObjectClass = try writer.importClass("com.snap.valdi.utils.ValdiMarshallableObject")

        let fullTypeName: String
        if let typeParameters = self.typeParameters, !typeParameters.isEmpty {
            let typeParamList = "<\(typeParameters.map(\.name).joined(separator: ", "))>"
            fullTypeName = "\(className)\(typeParamList)"
        } else {
            fullTypeName = className
        }

        let constructors = CodeWriter()
        let propertiesBody = CodeWriter()
        let fieldsBody = CodeWriter()

        writer.appendBody("\n")
        writer.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: comments))
        writer.appendBody("\n")

        let descriptorAnnotation: KotlinMarshallableObjectDescriptorAnnotation

        // ValdiMarshallableObject implements DisposablePrivate so we need to reserve the __dispose__() method
        _ = nameAllocator.allocate(property: "__dispose__")

        if isInterface {
            let proxyClassName = nameAllocator.allocate(property: "\(fullTypeName)_Proxy").name
            let marshallableInterface = try writer.importClass("com.snap.valdi.utils.ValdiMarshallable").name

            descriptorAnnotation = try writer.getDescriptorAnnotation(type: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.valdiInterface, typeParameters: typeParameters, additionalParameters: [("proxyClass", "\(proxyClassName)::class")])

            writer.appendBody(descriptorAnnotation)
            writer.appendBody("interface \(fullTypeName): \(marshallableInterface) {\n\n")
            writer.appendBody(propertiesBody)

            let marshallerClass = try writer.importClass("com.snap.valdi.utils.ValdiMarshaller").name
            writer.appendBody("""

            override fun pushToMarshaller(marshaller: \(marshallerClass)) = \(marshallableObjectClass.name).marshall(\(fullTypeName)::class.java, marshaller, this)
            \n
            """)

            writer.appendBody("}\n\n")

            writer.appendBody("class \(proxyClassName): \(fullTypeName) {\n\n")
            writer.appendBody(fieldsBody)
            writer.appendBody("\n\n")
            writer.appendBody(constructors)
            writer.appendBody("}")
        } else {
            descriptorAnnotation = try writer.getDescriptorAnnotation(type: KotlinMarshallableObjectDescriptorAnnotation.ObjectType.valdiClass, typeParameters: typeParameters, additionalParameters: [])

            writer.appendBody(descriptorAnnotation)
            writer.appendBody("class \(fullTypeName): \(marshallableObjectClass.name) {\n\n")
            writer.appendBody(propertiesBody)
            writer.appendBody("\n\n")
            writer.appendBody(fieldsBody)
            writer.appendBody("\n\n")
            writer.appendBody(constructors)
            writer.appendBody("}")
        }

        let kotlinProperties = resolveProperties(valdiProperties: properties)

        var resolvedNamesAndTypes = [ConstructorParameter]()
        for (propertyIndex, kotlinProperty) in kotlinProperties.enumerated() {
            let propertyName = kotlinProperty.propertyName
            let property = kotlinProperty.modelProperty
            let isOptional = property.type.isOptional

            let typeParser = try writer.getTypeParser(type: property.type.unwrappingOptional,
                                                      isOptional: isOptional,
                                                      propertyName: kotlinProperty.modelProperty.name,
                                                      nameAllocator: nameAllocator.scoped())

            let isFunction = property.type.unwrappingOptional.isFunction

            if let comments = property.comments {
                if usePublicFields {
                    // For public fields, place comments directly above the field declaration
                    fieldsBody.appendBody(FileHeaderCommentGenerator.generateMultilineComment(comment: comments))
                    fieldsBody.appendBody("\n")
                } else {
                    // For traditional approach, place comments in properties body
                    propertiesBody.appendBody(FileHeaderCommentGenerator.generateMultilineComment(comment: comments))
                    propertiesBody.appendBody("\n")
                }
            }

            let fieldVisibility = usePublicFields ? "var" : "private var"
            let fieldName = usePublicFields ? kotlinProperty.propertyName : kotlinProperty.fieldName
            let publicFieldName = isInterface ? (kotlinProperty.getterName ?? kotlinProperty.propertyName) : fieldName
            let expectedKotlinFieldName = isInterface ? (isFunction ? property.name : resolvePropertyGetterName(propertyName: property.name)) : "_\(property.name)"

            let fieldAnnotation = try descriptorAnnotation.appendField(propertyIndex: propertyIndex, propertyName: property.name, kotlinFieldName: publicFieldName, expectedKotlinFieldName: expectedKotlinFieldName, schemaBuilder: { schemaWriter in
                try schemaWriter.appendType(property.type, asBoxed: false, isMethod: isInterface)
            })

            if !isInterface {
                fieldsBody.appendBody(fieldAnnotation)
            }
            fieldsBody.appendBody("\(fieldVisibility) \(fieldName): \(typeParser.fullTypeName)\n\n")

            if isInterface {
                if isFunction {
                    guard let functionTypeParser = typeParser.functionTypeParser else {
                        throw CompilerError("Expected functionTypeParser for function property")
                    }
                    let parameterNamesString = functionTypeParser.parameterNames.map { $0.name }.joined(separator: ", ")

                    if isOptional {
                        let optionalMethodAnnotation = try writer.getJavaAnnotationContent(annotationType: "com.snap.valdi.schema.ValdiOptionalMethod")

                        propertiesBody.appendBody(optionalMethodAnnotation)
                        propertiesBody.appendBody("""
                            fun \(propertyName)\(functionTypeParser.methodTypeName) {
                              \(marshallableObjectClass.name).unimplementedMethod()
                            }
                            \n
                            """)
                    } else {
                        propertiesBody.appendBody("fun \(propertyName)\(functionTypeParser.methodTypeName)\n\n")
                    }

                    constructors.appendBody(fieldAnnotation)
                    constructors.appendBody("override fun \(propertyName)\(functionTypeParser.methodTypeName) {\n")
                    if isOptional {
                        if functionTypeParser.returnType.fullTypeName == "Unit" {
                            constructors.appendBody("    return \(fieldName)?.invoke(\(parameterNamesString)) ?: Unit")
                        } else {
                            constructors.appendBody("    return \(fieldName)!!(\(parameterNamesString))")
                        }
                    } else {
                        constructors.appendBody("    return \(fieldName)(\(parameterNamesString))")
                    }
                    constructors.appendBody("\n}\n\n")
                } else {
                    constructors.appendBody("override val \(propertyName): \(typeParser.fullTypeName)\n")
                    constructors.appendBody(fieldAnnotation)
                    constructors.appendBody("    get() = \(fieldName)\n\n")

                    propertiesBody.appendBody("val \(propertyName): \(typeParser.fullTypeName)\n")
                    if isOptional {
                        propertiesBody.appendBody("    get() = null\n\n")
                    } else {
                        propertiesBody.appendBody("\n")
                    }
                }
            } else {
                if !usePublicFields {
                    propertiesBody.appendBody("var \(propertyName): \(typeParser.fullTypeName)\n")
                    propertiesBody.appendBody("    get() = \(kotlinProperty.fieldName)\n")
                    propertiesBody.appendBody("    set(value) { \(kotlinProperty.fieldName) = value }\n\n")
                }
            }

            let constructorOmitted = property.omitConstructor?.android == true

            resolvedNamesAndTypes.append(ConstructorParameter(
                typeParser: typeParser,
                name: property.name,
                typeName: typeParser.fullTypeName,
                fieldName: fieldName,
                parameterToField: property.name,
                isFunction: isFunction,
                isOptional: property.type.isOptional,
                constructorOmitted: constructorOmitted))
        }

        try writeConstructors(writer: writer,
                              constructors: constructors,
                              constructorParameters: resolvedNamesAndTypes)
    }

    func write() throws -> [NativeSource] {
        try write(className: jvmClass.name, properties: properties, writer: writer, usePublicFields: usePublicFields)

        let data = try writer.content.indented(indentPattern: "    ").utf8Data()
        return [KotlinCodeGenerator.makeNativeSource(bundleInfo: bundleInfo, jvmClass: jvmClass, file: .data(data))]
    }
}
