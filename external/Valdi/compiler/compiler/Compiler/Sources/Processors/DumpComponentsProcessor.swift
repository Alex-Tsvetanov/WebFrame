//
//  DumpComponentsProcessor.swift
//  Compiler
//
//  Created by Simon Corsin on 1/14/19.
//  Copyright © 2019 Snap Inc. All rights reserved.
//

import Foundation

// passthrough
class DumpComponentsProcessor: CompilationProcessor {

    var description: String {
        return "Dumping Components"
    }

    private let outFileURLs: [URL]

    init(outFileURLs: [URL]) {
        self.outFileURLs = outFileURLs
    }

    private func dumpIOSType(iosType: IOSType) -> [String: Any] {
        return [
            "class": iosType.name,
            "import_statement": "#import \(iosType.importHeaderStatement)"
        ]
    }

    private func dumpAndroidClass(androidClassName: String) -> [String: Any] {
        let jvmClass = JVMClass(fullClassName: androidClassName)

        var out: [String: Any] = [
            "class": jvmClass.name,
            "full_class": androidClassName
        ]

        if let package = jvmClass.package {
            out["import_statement"] = "import \(package).\(jvmClass.name)"
        }

        return out
    }

    private func dumpComponent(result: CompilationResult, item: CompilationItem) -> [String: Any] {
        let rootNodeType = result.originalDocument.template?.rootNode?.nodeType ?? ""

        let iosView = IOSType(name: result.templateResult.rootElement.iosViewClass ?? "",
                              bundleInfo: item.bundleInfo,
                              kind: .class,
                              iosLanguage: item.bundleInfo.iosLanguage)

        var out: [String: Any] = [
            "relative_path": item.relativeBundleURL.path,
            "ios_view": dumpIOSType(iosType: iosView),
            "android_view": dumpAndroidClass(androidClassName: result.templateResult.rootElement.androidViewClass ?? "")
        ]
        if let rootViewNodeMapping = result.originalDocument.classMapping?.nodeMappingByClass[rootNodeType] {
            if let iosType = rootViewNodeMapping.iosType {
                out["ios_view"] = dumpIOSType(iosType: iosType)
            }
            if let androidClass = rootViewNodeMapping.androidClassName {
                out["android_view"] = dumpAndroidClass(androidClassName: androidClass)
            }
        }

        if let componentContextIOSType = result.originalDocument.componentContext?.iosType {
            out["ios_context"] = dumpIOSType(iosType: componentContextIOSType)
        }
        if let componentContextAndroidClass = result.originalDocument.componentContext?.androidClassName {
            out["android_context"] = dumpAndroidClass(androidClassName: componentContextAndroidClass)
        }

        if let viewModel = result.originalDocument.viewModel {
            if let viewModelIOSType = viewModel.iosType {
                out["ios_view_model"] = dumpIOSType(iosType: viewModelIOSType)
            }
            if let androidViewModelClass = viewModel.androidClassName {
                out["android_view_model"] = dumpAndroidClass(androidClassName: androidViewModelClass)
            }
        }

        return out
    }

    private func processDocuments(selectedItems: [SelectedItem<CompilationResult>]) throws -> [CompilationItem] {
        var out = [[String: Any]]()

        let itemsByBundleName = selectedItems.groupBy { (item) -> String in
            return item.item.bundleInfo.name
        }

        for (bundle, items) in itemsByBundleName {
            let outItems = items.map {
                dumpComponent(result: $0.data, item: $0.item)
            }

            out.append([
                "bundle": bundle,
                "path": items[0].item.bundleInfo.baseDir.path,
                "components": outItems])
        }

        let jsonData = try JSONSerialization.data(withJSONObject: out, options: .prettyPrinted)

        for url in outFileURLs {
            try jsonData.write(to: url)
        }

        return selectedItems.map { $0.item }
    }

    func process(items: CompilationItems) throws -> CompilationItems {
        return try items.select { (item) -> CompilationResult? in
            switch item.kind {
            case .document(let result):
                return result
            default:
                return nil
            }
        }.transformAll(processDocuments)
    }

}
