import Foundation

struct ResolvedModuleImport: CustomStringConvertible {
    /// Module name. nil for global modules
    let moduleName: String?

    /// Import path relative to the module root
    let moduleRelativeImportPath: String

    /// Import path relative to the root folder
    let rootRelativeImportPath: String

    /// Return `true` if the import is from the `test` subfolder, `false` otherwise
    var isTestImport: Bool {
        return self.moduleRelativeImportPath.starts(with: "test/")
    }

    init(compilationPath: String, importPath: String) {
        if importPath.hasPrefix("/") {
            self.rootRelativeImportPath = String(importPath[importPath.index(importPath.startIndex, offsetBy: 1)...])
        } else {
            self.rootRelativeImportPath = importPath
        }

        if self.rootRelativeImportPath.contains("/") {
            let rootRelativecomponents = self.rootRelativeImportPath.components(separatedBy: "/")
            let firstComponent = rootRelativecomponents.first!

            // NPM scoped import
            if (rootRelativecomponents.count >= 2 && firstComponent.starts(with: "@")) {
                self.moduleName = rootRelativecomponents[1] + rootRelativecomponents[0]
            } else {
                self.moduleName = firstComponent
            }

            self.moduleRelativeImportPath =  rootRelativecomponents[1...].joined(separator: "/")
        } else {
            self.moduleName = nil
            self.moduleRelativeImportPath = self.rootRelativeImportPath
        }
    }

    var description: String {
        return self.rootRelativeImportPath
    }
}
