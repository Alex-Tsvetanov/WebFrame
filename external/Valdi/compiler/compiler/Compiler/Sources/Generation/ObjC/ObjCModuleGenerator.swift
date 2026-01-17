// Copyright © 2024 Snap, Inc. All rights reserved.

final class ObjCModuleGenerator {
    private let iosType: IOSType
    private let exportedModule: ExportedModule
    private let bundleInfo: CompilationItem.BundleInfo
    private let classMapping: ResolvedClassMapping
    private let sourceFileName: GeneratedSourceFilename

    init(iosType: IOSType,
         exportedModule: ExportedModule,
         bundleInfo: CompilationItem.BundleInfo,
         classMapping: ResolvedClassMapping,
         sourceFileName: GeneratedSourceFilename) {
        self.iosType = iosType
        self.exportedModule = exportedModule
        self.bundleInfo = bundleInfo
        self.classMapping = classMapping
        self.sourceFileName = sourceFileName
    }

    private func generateCode(iosType: IOSType) throws -> GeneratedCode {
        let implementingType = self.iosType
        let header = ObjCFileGenerator()
        let impl = ObjCFileGenerator()

        header.addImport(path: "<valdi_core/SCValdiGeneratedModuleFactory.h>")
        header.addImport(path: "\"\(implementingType.importHeaderFilename(kind: .apiOnly))\"")

        let comments = """
        Subclasses of this class must implement the onLoadModule method
        and return an object implementing \(implementingType.name).
        """.trimmed

        header.appendBody(FileHeaderCommentGenerator.generateComment(sourceFilename: sourceFileName, additionalComments: comments))
        header.appendBody("""
        
        @interface \(iosType.name): SCValdiGeneratedModuleFactory
        
        - (id<\(implementingType.name)> _Nonnull)onLoadModule;
        
        @end
        """)

        impl.appendBody("""
        @implementation \(iosType.name)

        - (id<\(implementingType.name)>)onLoadModule
        {
          return [self onLoadModule];
        }
                
        - (NSString *)getModulePath
        {
          return @"\(exportedModule.modulePath)";
        }
        
        - (Protocol *)moduleProtocol
        {
          return @protocol(\(implementingType.name));
        }
        
        @end
        """)

        return GeneratedCode(apiHeader: header, apiImpl: impl, header: CodeWriter(), impl: CodeWriter())
    }

    func write() throws -> [NativeSource] {
        var output: [NativeSource] = []

        let iosGenerator = ObjCModelGenerator(iosType: iosType,
                                              bundleInfo: bundleInfo,
                                              typeParameters: nil,
                                              properties: exportedModule.model.properties,
                                              classMapping: classMapping,
                                              sourceFileName: sourceFileName,
                                              isInterface: true,
                                              emitLegacyConstructors: exportedModule.model.legacyConstructors,
                                              comments: exportedModule.model.comments)

        output += try iosGenerator.write()

        let resolvedIOSType = IOSType(name: "\(self.iosType.name)Factory", bundleInfo: bundleInfo, kind: self.iosType.kind, iosLanguage: self.iosType.iosLanguage)

        let code = try generateCode(iosType: resolvedIOSType)

        output += try NativeSource.iosNativeSourcesFromGeneratedCode(code, iosType: resolvedIOSType, bundleInfo: bundleInfo)

        return output
    }

}
