//
//  TypeScriptProcessingUtils.swift
//  Compiler
//
//  Created by saniul on 17/06/2019.
//

import Foundation

struct CompilationResultAndItemIndex {
    var compilationResult: CompilationResult
    let itemIndex: Int
}

class DocumentsIndexer {
    private var items: [CompilationItem] = []
    private let lock = DispatchSemaphore.newLock()
    private var itemIndexByUserScriptSourceURL = [URL: Int]()

    var allItems: [CompilationItem] {
        get { return lock.lock { items } }
    }

    init(items: [CompilationItem]) {
        for item in items {
            doAppend(item: item)
        }
    }

    func append(item: CompilationItem) {
        let _ = lock.lock {
            doAppend(item: item)
        }
    }

    func replace(atIndex index: Int, block: (CompilationItem) -> CompilationItem) {
        lock.lock {
            let oldItem = self.items[index]
            let newItem = block(oldItem)

            self.items[index] = newItem

            onReplace(oldItem: oldItem, newItem: newItem)
        }
    }

    private func onReplace(oldItem: CompilationItem, newItem: CompilationItem) {
        if case .document(let document) = oldItem.kind {
            if case .document = newItem.kind {
                // Nothing to do
                return
            }
            // We had a document and we dont anymore, remove the userScriptSourceURL

            if let userScriptSourceURL = document.userScriptSourceURL {
                self.itemIndexByUserScriptSourceURL.removeValue(forKey: userScriptSourceURL)
            }
        }
    }

    func injectError(logger: ILogger, _ error: Error, relatedItem: CompilationItem) {
        let sourceURL = relatedItem.sourceURL

        lock.lock {
            var found = false
            items.transform { (item) in
                if case .javaScript = item.kind, item.sourceURL == sourceURL {
                    let newItem = item.with(error: error)
                    onReplace(oldItem: item, newItem: newItem)
                    item = newItem
                    found = true
                } else if case .document(let result) = item.kind, result.userScriptSourceURL == sourceURL {
                    let newItem = item.with(error: error)
                    onReplace(oldItem: item, newItem: newItem)
                    item = newItem
                    logger.error("TypeScript from Document \(item.sourceURL.path) had an error: \(error.legibleLocalizedDescription)")
                    found = true
                }
            }
            if !found {
                items.append(relatedItem.with(error: error))
            }
        }
    }

    @discardableResult private func doAppend(item: CompilationItem) -> Int {
        let itemsIndex = items.count

        switch item.kind {
        case .document(let result):
            if let userScriptSourceURL = result.userScriptSourceURL {
                self.itemIndexByUserScriptSourceURL[userScriptSourceURL] = itemsIndex
            }
        default:
            break
        }

        self.items.append(item)

        return itemsIndex
    }

    func findDocument(fromSourceURL sourceURL: URL) -> CompilationResultAndItemIndex? {
        return lock.lock {
            return doFindDocument(fromSourceURL: sourceURL)
        }
    }

    func findOrCreateDocument(fromSourceURL sourceURL: URL, compilationItem: CompilationItem) -> CompilationResultAndItemIndex {
        return lock.lock {
            if let found = doFindDocument(fromSourceURL: sourceURL) {
                return found
            }

            let componentPath = ComponentPath(fileName: "__TMP__(\(compilationItem.relativeProjectPath))", exportedMember: "__EXPORTED_MEMBER__")
            let fakeCompilationResult = CompilationResult(componentPath: componentPath,
                                                          originalDocument: ValdiRawDocument(),
                                                          templateResult: TemplateCompilerResult(rootElement: ValdiJSElement(id: "", componentPath: nil, nodeType: "__ROOT__", jsxName: nil), actions: []),
                                                          classMapping: ResolvedClassMapping(localClassMapping: nil, projectClassMapping: ProjectClassMapping(allowMappingOverride: false), currentBundle: compilationItem.bundleInfo),
                                                          userScriptSourceURL: sourceURL,
                                                          scriptLang: "tsx")
            let itemIndex = doAppend(item: compilationItem.with(newKind: .document(fakeCompilationResult)))

            return CompilationResultAndItemIndex(compilationResult: fakeCompilationResult, itemIndex: itemIndex)
        }
    }

    private func doFindDocument(fromSourceURL sourceURL: URL) -> CompilationResultAndItemIndex? {
        guard let itemIndex = itemIndexByUserScriptSourceURL[sourceURL] else {
            return nil
        }

        guard case .document(let result) = items[itemIndex].kind else {
            fatalError("this should not happen")
        }

        return CompilationResultAndItemIndex(compilationResult: result, itemIndex: itemIndex)
    }
}
