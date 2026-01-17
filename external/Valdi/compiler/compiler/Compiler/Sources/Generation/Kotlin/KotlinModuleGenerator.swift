// Copyright © 2024 Snap, Inc. All rights reserved.

final class KotlinModuleGenerator {
    private let bundleInfo: CompilationItem.BundleInfo
    private let fullTypeName: String
    private let exportedModule: ExportedModule
    private let classMapping: ResolvedClassMapping
    private let sourceFileName: GeneratedSourceFilename

    init(bundleInfo: CompilationItem.BundleInfo,
         fullTypeName: String,
         exportedModule: ExportedModule,
         classMapping: ResolvedClassMapping,
         sourceFileName: GeneratedSourceFilename) {
        self.bundleInfo = bundleInfo
        self.fullTypeName = fullTypeName
        self.exportedModule = exportedModule
        self.classMapping = classMapping
        self.sourceFileName = sourceFileName
    }

    func write() throws -> [NativeSource] {
        var output: [NativeSource] = []
        let androidGenerator = KotlinModelGenerator(bundleInfo: bundleInfo,
                                                    className: fullTypeName,
                                                    typeParameters: exportedModule.model.typeParameters,
                                                    properties: exportedModule.model.properties,
                                                    classMapping: classMapping,
                                                    sourceFileName: sourceFileName,
                                                    isInterface: exportedModule.model.exportAsInterface,
                                                    emitLegacyConstructors: exportedModule.model.legacyConstructors,
                                                    comments: exportedModule.model.comments)
        output += try androidGenerator.write()

        let jvmClass = JVMClass(fullClassName: "\(fullTypeName)Factory")

        let generator = KotlinCodeGenerator(package: jvmClass.package, classMapping: classMapping)
        let baseClass = try generator.importClass("com.snap.valdi.modules.ValdiGeneratedModuleFactory")
        let implementingClass = try generator.importClass(fullTypeName)

        generator.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: exportedModule.model.comments))
        generator.appendBody("\n")
        generator.appendBody("""
        abstract class \(jvmClass.name): \(baseClass.name)<\(implementingClass.name)>() {
            override fun getModulePath(): String {
               return "\(exportedModule.modulePath)"
            }
        }
        """)

        let data = try generator.content.indented(indentPattern: "    ").utf8Data()

        output.append(KotlinCodeGenerator.makeNativeSource(bundleInfo: bundleInfo, jvmClass: jvmClass, file: .data(data)))

        return output
    }

}
