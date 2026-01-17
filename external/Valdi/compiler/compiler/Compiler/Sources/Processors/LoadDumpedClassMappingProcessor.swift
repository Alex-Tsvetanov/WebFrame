import Foundation

class LoadCompilationMetadataProcessor: CompilationProcessor {

    let projectClassMappingManager: ProjectClassMappingManager

    var description: String {
        return "Load serialized compilation metadata"
    }

    private let typeScriptCompilationManager: TypeScriptCompilerManager
    private let typeScriptNativeTypeResolver: TypeScriptNativeTypeResolver

    init(projectClassMappingManager: ProjectClassMappingManager, typeScriptCompilationManager: TypeScriptCompilerManager, typeScriptNativeTypeResolver: TypeScriptNativeTypeResolver) {
        self.projectClassMappingManager = projectClassMappingManager
        self.typeScriptCompilationManager = typeScriptCompilationManager
        self.typeScriptNativeTypeResolver = typeScriptNativeTypeResolver
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        let decoder = JSONDecoder()

        let selectedItems = items.select { item -> File? in
            guard case let .compilationMetadata(file) = item.kind else {
                return nil
            }

            return file
        }

        return try selectedItems.transformAll { selectedItems in
            for selectedItem in selectedItems {
                let file = selectedItem.data

                let decoded = try decoder.decode(CompilationMetadata.self, from: try file.readData())
                try projectClassMappingManager.mutate { mapping in
                    for (k, v) in decoded.classMappings {
                        mapping.registerResolved(fullyQualifiedName: k, klass: v)
                    }
                }


                try typeScriptNativeTypeResolver.restore(fromSerialized: decoded.nativeTypes)
            }
            return []
        }
    }

}
