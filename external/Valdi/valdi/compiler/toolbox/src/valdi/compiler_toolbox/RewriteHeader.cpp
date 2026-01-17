#include "valdi/compiler_toolbox/RewriteHeader.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/TextParser.hpp"

namespace Valdi {

static bool shouldRenameImportComponent(const StringBox& component,
                                        const std::vector<StringBox>& preservedModuleNames) {
    for (const auto& preservedModuleName : preservedModuleNames) {
        if (preservedModuleName == component) {
            return false;
        }
    }
    return true;
}

Result<std::string> rewriteImport(TextParser& parser,
                                  const StringBox& moduleName,
                                  const std::vector<StringBox>& preservedModuleNames,
                                  bool flattenImportPaths) {
    parser.tryParseWhitespaces();

    std::optional<std::string_view> term;
    bool angled = false;
    if (parser.tryParse('"')) {
        term = parser.readUntilCharacterAndParse('"');
    } else if (parser.parse('<')) {
        term = parser.readUntilCharacterAndParse('>');
        angled = true;
    }

    if (!term) {
        return parser.getError();
    }

    auto importPath = StringCache::getGlobal().makeString(term.value());
    auto components = importPath.split('/');

    if (components.size() > 1 && shouldRenameImportComponent(components[0], preservedModuleNames)) {
        components[0] = moduleName;

        if (flattenImportPaths && components.size() > 2) {
            components = {components[0], components[components.size() - 1]};
        }
    }

    std::string output;
    if (angled) {
        output += "<";
        output += StringBox::join(components, "/").toStringView();
        output += ">";
    } else {
        output += "\"";
        output += StringBox::join(components, "/").toStringView();
        output += "\"";
    }

    return output;
}

Result<Void> rewriteHeader(const StringBox& inputFile,
                           const StringBox& outputFile,
                           const StringBox& moduleName,
                           const std::vector<StringBox>& preservedModuleNames,
                           bool flattenImportPaths) {
    auto inputFileData = DiskUtils::load(Path(inputFile));
    if (!inputFileData) {
        return inputFileData.moveError();
    }

    std::string output;

    TextParser parser(inputFileData.value().asStringView());

    while (!parser.isAtEnd()) {
        if (parser.tryParse("#include")) {
            auto rewritten = rewriteImport(parser, moduleName, preservedModuleNames, flattenImportPaths);
            if (!rewritten) {
                return rewritten.moveError();
            }

            output += "#include " + rewritten.value();
        } else if (parser.tryParse("#import")) {
            auto rewritten = rewriteImport(parser, moduleName, preservedModuleNames, flattenImportPaths);
            if (!rewritten) {
                return rewritten.moveError();
            }

            output += "#import " + rewritten.value();
        } else {
            output += parser.readUntilCharacter('\n');
            if (parser.tryParse('\n')) {
                output.append(1, '\n');
            }
        }
    }

    return DiskUtils::store(Path(outputFile), output);
}

} // namespace Valdi