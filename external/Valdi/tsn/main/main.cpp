#include "tsn/tsn.h"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace Valdi;

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    for (char ch : s) {
        if (ch == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += ch;
        }
    }
    // Add the last token
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, const char** argv) {
    if (argc <= 1) {
        printf("Should specify the list of modules to load en evaluate\n");
        return EXIT_FAILURE;
    }

    auto context = tsn_init(argc, argv);
    if (context == nullptr) {
        std::cout << "Failed to init tsn" << std::endl;
        return EXIT_FAILURE;
    }

    auto global = JS_GetGlobalObject(context);

    // Create global var scriptArgs for command line arguments
    auto scriptArgs = JS_NewArray(context);
    JS_SetPropertyStr(context, global, "scriptArgs", scriptArgs);

    for (int i = 1; i < argc; i++) {
        // Parse command line
        auto command = split(std::string(argv[i]), ' ');
        const auto& moduleName = command[0];
        // Add script name to scriptArgs[0]
        auto argObj = JS_NewString(context, moduleName.data());
        JS_SetPropertyUint32(context, scriptArgs, 0, argObj);
        // Add arguments
        for (size_t j = 1; j < command.size(); ++j) {
            auto argObj = JS_NewString(context, command[j].data());
            JS_SetPropertyUint32(context, scriptArgs, j, argObj);
        }

        auto path = Path(std::string_view(moduleName.data()));

        auto moduleNameWithoutExtension = Path(path).removeFileExtension().toString();

        if (!tsn_contains_module(moduleNameWithoutExtension.c_str())) {
            // Interpreter mode
            VALDI_INFO(ConsoleLogger::getLogger(), "Evaluating module '{}' as interpreted", path.toString());

            auto loadResult = DiskUtils::load(path);
            if (!loadResult) {
                VALDI_ERROR(ConsoleLogger::getLogger(), "Failed to load module: {}", loadResult.error());
                tsn_fini(&context);
                return EXIT_FAILURE;
            }

            auto moduleData = loadResult.moveValue();

            auto script = std::string(reinterpret_cast<const char*>(moduleData.data()), moduleData.size());
            auto retval = tsn_eval(context, script.data(), script.size(), moduleName.data(), 0);
            if (tsn_is_exception(context, retval)) {
                std::cout << "Failed to load module" << std::endl;
                tsn_dump_error(context);
                tsn_fini(&context);
                return EXIT_FAILURE;
            }

            tsn_release(context, retval);
        } else {
            // Native mode
            VALDI_DEBUG(ConsoleLogger::getLogger(), "Evaluating module '{}' as native", path.toString());

            auto loadedModule = tsn_require_from_string(context, moduleNameWithoutExtension.c_str());
            if (tsn_is_exception(context, loadedModule)) {
                std::cout << "Failed to load module" << std::endl;
                tsn_dump_error(context);
                tsn_fini(&context);
                return EXIT_FAILURE;
            }

            tsn_release(context, loadedModule);
        }

        if (tsn_execute_pending_jobs(tsn_get_runtime(context)) < 0) {
            tsn_dump_error(context);
            tsn_fini(&context);
            return EXIT_FAILURE;
        }
    }
    JS_FreeValue(context, global);

    tsn_fini(&context);
    return EXIT_SUCCESS;
}
