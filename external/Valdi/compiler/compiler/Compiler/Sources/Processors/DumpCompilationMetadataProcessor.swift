import Foundation

class DumpCompilationMetadataProcessor: CompilationProcessor {

    let projectConfig: ValdiProjectConfig
    let compilerConfig: CompilerConfig
    let projectClassMappingManager: ProjectClassMappingManager

    var description: String {
        return "Dumping compilation metadata"
    }

    private let typeScriptCompilationManager: TypeScriptCompilerManager
    private let typeScriptNativeTypeResolver: TypeScriptNativeTypeResolver

    init(projectConfig: ValdiProjectConfig, compilerConfig: CompilerConfig, projectClassMappingManager: ProjectClassMappingManager, typeScriptCompilationManager: TypeScriptCompilerManager, typeScriptNativeTypeResolver: TypeScriptNativeTypeResolver) {
        self.projectConfig = projectConfig
        self.compilerConfig = compilerConfig
        self.projectClassMappingManager = projectClassMappingManager
        self.typeScriptCompilationManager = typeScriptCompilationManager
        self.typeScriptNativeTypeResolver = typeScriptNativeTypeResolver
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        let resolvedMappings = projectClassMappingManager.copyProjectClassMapping().copyMappings()
        let nativeTypes =  typeScriptNativeTypeResolver.serialize()

        let encoder = JSONEncoder()
        encoder.outputFormatting = .sortedKeys

        let selectedItems = items.select { item -> ([String: ValdiClass], SerializedTypeScriptNativeTypeResolver)? in
            guard case .moduleYaml = item.kind, !compilerConfig.onlyFocusProcessingForModules.isEmpty && compilerConfig.onlyFocusProcessingForModules.contains(item.bundleInfo.name) else {
                return nil
            }
            let resolvedMappingsForModule = resolvedMappings.filter { (_, v) in
                v.sourceModuleName == item.bundleInfo.name
            }

            let filteredEntries = nativeTypes.entries.filter { entry in
                entry.emittingBundleName == item.bundleInfo.name
            }

            let resolvedTypeResolverForModule = SerializedTypeScriptNativeTypeResolver(entries: filteredEntries)

            return (resolvedMappingsForModule, resolvedTypeResolverForModule)
        }

        let transformed = try selectedItems.transformEach { selectedItem in
            let item = selectedItem.item
            let (resolvedMappingsFromModule, resolvedTypeResolverForModule) = selectedItem.data
            let compilationMetadata = CompilationMetadata(classMappings: resolvedMappingsFromModule, nativeTypes: resolvedTypeResolverForModule)

            let encoded = try encoder.encode(compilationMetadata)
            let file = File.data(encoded)

            return [item, item.with(newKind: .compilationMetadata(file))]
        }

        return transformed
    }

}
