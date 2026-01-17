// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

class StaticResGenerator {

    static func generate(baseUrl: URL, inputFiles: [String], to path: String) throws {
        let url = baseUrl.resolving(path: path)

        if FileManager.default.fileExists(atPath: url.path) {
            try? FileManager.default.removeItem(atPath: url.path)
        }

        let writer = CppFileGenerator(namespace: CPPNamespace(components: ["snap", "valdi", "static_resources"]), isHeader: false)

        writer.includeSection.addInclude(path: "valdi_core/cpp/Resources/StaticResourceRegistry.hpp")

        let staticArraysSection = CppCodeWriter()

        writer.body.appendBody(staticArraysSection)

        writer.body.appendBody("__attribute__((constructor)) static void register_static_valdi_resources() {\n")

        let registerSection = CppCodeWriter()
        writer.body.appendBody(registerSection)

        writer.body.appendBody("}")

        let propertyNameAllocator = PropertyNameAllocator()

        for inputFile in inputFiles {
            let fileURL = baseUrl.resolving(path: inputFile)

            let fileData = try File.url(fileURL).readData()

            let filePath = fileURL.lastPathComponent

            let resolvedFileData: Data
            let requiresDecompression: Bool

            if filePath.hasSuffix(".\(FileExtensions.json)") {
                // Automatically compress json files
                resolvedFileData = try ZstdCompressor.compress(data: fileData)
                requiresDecompression = true
            } else {
                resolvedFileData = fileData
                requiresDecompression = false
            }

            let fileContentStr = resolvedFileData.lazy.map { String(format: "0x%02x", $0) }.joined(separator: ", ")

            let variableName = propertyNameAllocator.allocate(property: filePath.snakeCased).name

            staticArraysSection.appendBody("static uint8_t \(variableName)[] = {\n\(fileContentStr)\n};\n\n")
            registerSection.appendBody("VALDI_REGISTER_STATIC_RESOURCE(\"\(filePath)\", &\(variableName)[0], \(resolvedFileData.count), \(requiresDecompression ? "true" : "false"));\n")
        }

        let data = try writer.content.indented.utf8Data()

        try data.write(to: url, options: .atomic)
    }

}
