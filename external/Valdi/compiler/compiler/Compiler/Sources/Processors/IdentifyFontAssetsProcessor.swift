// Copyright © 2024 Snap, Inc. All rights reserved.

class IdentifyFontAssetsProcessor: CompilationProcessor {

    var description: String {
        return "Identifying Font assets"
    }

    private func identifyFont(selectedItem: SelectedItem<File>) -> CompilationItem {
        let relativeBundleURL = selectedItem.item.relativeBundleURL

        let outputFilename = "\(relativeBundleURL.deletingPathExtension().lastPathComponent.snakeCased).\(relativeBundleURL.pathExtension)"

        return selectedItem.item.with(newKind: .resourceDocument(DocumentResource(outputFilename: outputFilename, file: selectedItem.data)))
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return items.select { item -> File? in
            if case .rawResource(let file) = item.kind, FileExtensions.fonts.contains(item.sourceURL.pathExtension) {
                return file
            }
            return nil
        }.transformEach(identifyFont)
    }

}
