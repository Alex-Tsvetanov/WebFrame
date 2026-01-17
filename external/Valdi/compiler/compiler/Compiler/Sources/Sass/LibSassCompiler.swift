import Foundation
import Clibsass

public struct SassResult {
    let output: String
    let includedFilePaths: [String]
}

public final class LibSassCompiler {
    // Right now this is a dumb simple API so no initialization, just a single compile input -> output function
    private init() { }

    public static func compile(inputString: String, includePath: String) throws -> SassResult {
        typealias DataContext = OpaquePointer

        var inputCString = inputString.utf8CString

        return try inputCString.withUnsafeMutableBytes { input -> SassResult in
            let strlen = input.count
            let inputCopy = malloc(strlen)!
            memcpy(inputCopy, input.baseAddress!.bindMemory(to: Int8.self, capacity: strlen), strlen)
            let context: DataContext = sass_make_data_context(inputCopy.bindMemory(to: Int8.self, capacity: strlen))
            defer { sass_delete_data_context(context) }

            let options = sass_data_context_get_options(context)
            // TODO: set desired options
            sass_option_set_precision(options, 5)
            sass_option_set_source_comments(options, false)
            sass_option_set_output_style(options, SASS_STYLE_COMPACT)
            sass_option_set_include_path(options, includePath)
            sass_data_context_set_options(context, options)

            let compiler = sass_make_data_compiler(context)
            defer { sass_delete_compiler(compiler) }
            sass_compiler_parse(compiler)
            sass_compiler_execute(compiler)

            let errorStatus = sass_context_get_error_status(context)
            guard errorStatus == 0 else {
                let errorFile = sass_context_get_error_file(context).map(String.init(cString:))
                // The error might've occurred in an _included_ file, instead of the original document
                let errorFileURL = errorFile?.nonEmpty.flatMap(URL.init(fileURLWithPath:))
                let failedDependenciesURLs: Set<URL> = errorFileURL.map { [$0] } ?? []

                // LibSass 3.6.2 has a heap-use-after-free bug that causes sass_context_get_error_src to sometimes
                // return an invalid pointer see: https://github.com/sass/libsass/issues/3019
                // FIXME: Reenable this code path after LibSass 3.6.3 is released
                #if LIB_SASS_ERROR_SRC_SAFE_TO_USE
                let message = sass_context_get_error_text(context).map(String.init(cString:)) ?? "<invalid error message>"
                let errorSource = sass_context_get_error_src(context).map(String.init(cString:)) ?? "<invalid source>"

                let errorLine = sass_context_get_error_line(context)
                let errorColumn = sass_context_get_error_column(context)

                let errorFileRelativePath = errorFileURL?.map { $0.relativePath(from: URL(fileURLWithPath: includePath)) }
                let inFileSuffix = errorFileRelativePath.map { " in \($0)" } ?? ""

                throw CompilerError(type: "Sass error" + inFileSuffix,
                                         message: message,
                                         atZeroIndexedLine: errorLine - 1,
                                         column: errorColumn,
                                         inDocument: sourceWithError,
                                         failedDependenciesURLs: failedDependenciesURLs)
                #else
                let message = sass_context_get_error_message(context).map(String.init(cString:)) ?? "<invalid error message>"
                throw CompilerError("Sass error: \(message)", failedDependenciesURLs: failedDependenciesURLs)
                #endif
            }

            let output = sass_context_get_output_string(context)
            let outputString = output.map(String.init(cString:)) ?? ""

            var includedFilePaths = [String]()

            if var includedFiles = sass_context_get_included_files(context) {
                var index = 0
                while index < sass_context_get_included_files_size(context) {
                    if let filePath = includedFiles.pointee {
                        includedFilePaths.append(String(cString: filePath))
                    }
                    index += 1
                    includedFiles = includedFiles.advanced(by: 1)
                }
            }

            return SassResult(output: outputString, includedFilePaths: includedFilePaths)
        }
    }

    public struct Error: LocalizedError {
        public let code: Code
        public let message: String

        public init(code: Int32, message: String) {
            self.code = Error.Code(rawValue: code) ?? .unknown
            self.message = message
        }

        public enum Code: Int32 {
            case normal = 1
            case memoryError
            case untranslatedException
            case legacyStringException
            case unknown
        }

        public var errorDescription: String? {
            return "libsass error code \(code.rawValue): \(message)"
        }

        public var failureReason: String? {
            return message
        }
    }

}
