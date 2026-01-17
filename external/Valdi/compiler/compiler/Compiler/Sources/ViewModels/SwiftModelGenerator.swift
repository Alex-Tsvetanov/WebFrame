//  Copyright © 2024 Snap Inc. All rights reserved.

import Foundation

final class SwiftModelGenerator {
    private let iosType: IOSType
    private let bundleInfo: CompilationItem.BundleInfo
    private let className: String
    private let typeParameters: [ValdiTypeParameter]?
    private let properties: [ValdiModelProperty]
    private let classMapping: ResolvedClassMapping
    private let sourceFileName: GeneratedSourceFilename
    private let isInterface: Bool
    private let comments: String?
    private let nameAllocator = PropertyNameAllocator.forSwift()

    init(iosType: IOSType, bundleInfo: CompilationItem.BundleInfo, typeParameters: [ValdiTypeParameter]?, properties: [ValdiModelProperty], classMapping: ResolvedClassMapping, sourceFileName: GeneratedSourceFilename, isInterface: Bool, comments: String?) {
        self.iosType = iosType
        self.bundleInfo = bundleInfo
        self.className = iosType.swiftName
        self.typeParameters = typeParameters
        self.properties = properties
        self.classMapping = classMapping
        self.sourceFileName = sourceFileName
        self.isInterface = isInterface
        self.comments = comments
    }

    private struct SwiftGeneratedCode {
        let filename: String
        let source: SwiftSourceFileGenerator
    }

    private func resolveProperties(valdiProperties: [ValdiModelProperty], _ classGenerator: SwiftSourceFileGenerator) throws -> [SwiftProperty] {
        return try valdiProperties.enumerated().map { (index, property) throws in
            // let propertyName = nameAllocator.allocate(property: property.name).name
            let propertyName = nameAllocator.allocate(property: property.name).name
            let typeParser = try classGenerator.getTypeParser(type: property.type.unwrappingOptional,
                                                              isOptional: property.type.isOptional,
                                                              functionHelperName: propertyName,
                                                              nameAllocator: classGenerator.nameAllocator.scoped())
            return SwiftProperty(name: propertyName,
                                 typeParser: typeParser,
                                 comments: property.comments)
        }
    }

    private func generateInterface(properties: [SwiftProperty], classGenerator: SwiftSourceFileGenerator) throws -> CodeWriter {
        let interface = CodeWriter()
        let interfaceBody = try properties.map() { property in
            let commentWriter = CodeWriter()
            if let comments = property.comments {
                commentWriter.appendBody(FileHeaderCommentGenerator.generateMultilineComment(comment: comments))
                commentWriter.appendBody("\n")
            }

            if (!property.typeParser.isFunction) {
                return "\(commentWriter.content)var \(property.name): \(property.typeParser.typeName) { get }"
            } else {
                guard let functionType = property.typeParser.functionTypeParser?.methodTypeWithNames else {
                    throw CompilerError("Cannot get function signature for property \(property.name)")
                }
                return "\(commentWriter.content)func \(property.name)\(functionType)"
            }
        }.joined(separator: indent(spaces: 8))
        
        // TODO(3521): Update to VladiMarshall*
        interface.appendBody(
        """
        public protocol \(className) : ValdiMarshallableInterface {
            \(interfaceBody)
        }\n\n
        """)
        return interface
    }

    private func generateCode(for valdiModelProperties: [ValdiModelProperty], className: String) throws -> SwiftGeneratedCode {
        let thisModule = self.iosType.importPrefix ?? ""
        let classGenerator = SwiftSourceFileGenerator(className: className, moduleName: thisModule)
        // TODO(3521): Update to ValdiCoreSwift
        classGenerator.addImport(path: "ValdiCoreSwift")
        classGenerator.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: self.comments))
        classGenerator.appendBody("\n")

        let nameAllocator = classGenerator.nameAllocator
        _ = nameAllocator.allocate(property: className)

        let properties = try resolveProperties(valdiProperties: valdiModelProperties, classGenerator)

        let variableDeclarations = SwiftSourceFileGenerator.writeVariableDeclaration(properties: properties, isInterface: isInterface)
        let classConstructor = SwiftSourceFileGenerator.writeConstructor(properties: properties)
        let objectMarshaller = SwiftSourceFileGenerator.writeObjectMarshaller(properties: properties, className: className, isInterface: isInterface)
        let objectUnmarshaller = SwiftSourceFileGenerator.writeObjectUnmarshaller(properties: properties, isInterface: isInterface)
        let objectDescriptorGetter = try SwiftSourceFileGenerator.writeObjectDescriptorGetter(
            properties: properties,
            propertyTypes: valdiModelProperties.map { $0.type },
            typeParameters: self.typeParameters,
            objectDescriptorType: isInterface ? "ValdiMarshallableObjectType.Interface" : "ValdiMarshallableObjectType.Class")
        let functionHelpers = try SwiftSourceFileGenerator.writeFunctionHelpers(sourceGenerator: classGenerator)
        let typeParameterList = self.typeParameters?.map { $0.name }.joined(separator: ", ") ?? ""
        let typeParameterString =  self.typeParameters?.isEmpty == false ? "<\(typeParameterList)>" : ""

        if (!isInterface) {
            let classDefinition =
            """
            public class \(className)\(typeParameterString) : ValdiMarshallableObject {
            \(variableDeclarations.content)
            \(classConstructor.content)
            \(objectMarshaller.content)
            \(objectUnmarshaller.content)
            \(functionHelpers.content)
            \(objectDescriptorGetter.content)
            }\n
            """
            classGenerator.appendBody(classDefinition)
        } else {
            let interface = try generateInterface(properties: properties, classGenerator: classGenerator)
            let interfaceDefinition =
            """
            \(interface.content)
            extension \(className) {
            \(objectMarshaller.content)
            \(functionHelpers.content)
            }
            public class \(className)_Proxy : \(className), ValdiMarshallableObject {
            \(variableDeclarations.content)
            \(objectUnmarshaller.content)
            \(objectDescriptorGetter.content)
            }\n
            """
            classGenerator.appendBody(interfaceDefinition)
        }

        let dummyParameterList = self.typeParameters?.map { p in "Void" }.joined(separator: ", ") ?? ""
        let dummyParameterString = self.typeParameters?.isEmpty == false ? "<\(dummyParameterList)>" : ""
        let descriptorNamespace = (isInterface ? "\(className)_Proxy" : className) + dummyParameterString
        let registrationFunction = 
        """
        public func register_\(className)() {
            ValdiMarshallableObjectRegistry.shared.register(className: "\(className)", objectDescriptor: \(descriptorNamespace).getDescriptor())
        }\n
        """
        classGenerator.appendBody(registrationFunction)

        return SwiftGeneratedCode(filename: className, source: classGenerator)
    }

    func write() throws -> [NativeSource] {
        let generatedCode = try generateCode(for: properties, className: className)
        let source = try generatedCode.source.content.utf8Data()
        let sourcesDirectoryRelativePath = "../\(bundleInfo.iosModuleName)\(IOSType.HeaderImportKind.apiOnlyModuleNameSuffix)"
        return [NativeSource(relativePath: sourcesDirectoryRelativePath,
                             filename: "\(generatedCode.filename).\(FileExtensions.swift)",
                             file: .data(source),
                             groupingIdentifier: "\(bundleInfo.iosModuleName)\(IOSType.HeaderImportKind.apiOnlyModuleNameSuffix).\(FileExtensions.swift)",
                             groupingPriority: isInterface ? IOSType.interfaceTypeGroupingPriority : IOSType.classTypeGroupingPriority)]
    }
}
