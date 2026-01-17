#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "test262/shared/Test262Helpers.hpp"
#include "test262/shared/Test262Serialization.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "TestReporter.hpp"
#include <sstream>
#include <stdexcept>
#include <tsn/tsn.h>

using namespace std;
using namespace Valdi;
namespace fs = filesystem;

const string kSTAFile = "sta.js";
const string kAssertFile = "assert.js";
const string kDonePrintHandle = "doneprintHandle.js";

// NOTE(rjaber): There are some bug in current compilation, and this is temp measure to avoid hitting assert / heap
// corruption / segfaults in the typescript VM
static set<std::string> decodeSkipList(const fs::path& skipListFile) {
    auto skipListResult = DiskUtils::load(Path(skipListFile.string()));
    if (!skipListResult) {
        TEST262_LOG(cerr, "Failed to save file {} with error {}", skipListFile, skipListResult.error().getMessage());
        return {};
    }
    const auto skipListData = skipListResult.moveValue();
    auto skipListValueResult = jsonToValue(skipListData.data(), skipListData.size());
    if (!skipListValueResult) {
        TEST262_LOG(
            cerr, "Failed to parse file {} with error {}", skipListFile, skipListValueResult.error().getMessage());
        return {};
    }
    const auto skipListValue = skipListValueResult.moveValue();
    if (!skipListValue.isArray()) {
        TEST262_LOG(cerr, "Parsed file {} expected top level array", skipListFile);
        return {};
    }

    set<string> retval;
    for (const auto& skip : *skipListValue.getArrayRef()) {
        if (!skip.isString()) {
            TEST262_LOG(cerr, "skip list element for {} is not a string", skipListFile);
        }

        retval.insert(skip.toString());
    }

    return retval;
}

static void decodeException(JSContext* ctx, std::string& errorType, std::string& errorDescription) {
    JSValue exception_val;
    exception_val = tsn_get_exception(ctx);
    if (tsn_is_error(ctx, exception_val)) {
        auto val = tsn_get_error_type(ctx, exception_val);
        errorType = tsn_to_cstring(ctx, val);
        val = tsn_get_error_description(ctx, exception_val);
        errorDescription = tsn_to_cstring(ctx, val);
    } else {
        errorType = "Unknown";
        errorDescription = "No Exception Found";
    }
    tsn_release(ctx, exception_val);
}

class realm {
public:
    realm() {
        if (_context != nullptr) {
            return;
        }
        _context = tsn_init(0, nullptr);
    }

    bool evaluateHarness(const std::string& file,
                         std::string source,
                         std::string& error_type,
                         std::string& error_description) {
        if (_context == nullptr) {
            return true;
        }

        auto val = tsn_eval(_context, source.c_str(), source.length(), source.c_str(), TSN_EVAL_TYPE_GLOBAL);
        if (tsn_is_exception(_context, val) != 0) {
            decodeException(_context, error_type, error_description);
            return false;
        }
        tsn_release(_context, val);
        return true;
    }

    bool evaluateCompiledTest(
        std::string& module, bool isStrict, bool isModule, std::string& error_type, std::string& error_description) {
        if (_context == nullptr) {
            return true;
        }

        // TODO(rjaber): Add support for module/global loading, as well as isStrict enable/disable
        auto val = tsn_load_module(_context, module.c_str());
        if (tsn_is_exception(_context, val) != 0) {
            decodeException(_context, error_type, error_description);
            return false;
        }
        auto loadedModule = tsn_call(_context, val, tsn_undefined(_context), 0, NULL);
        if (tsn_is_exception(_context, val) != 0) {
            decodeException(_context, error_type, error_description);
            return false;
        }

        tsn_release(_context, loadedModule);
        tsn_release(_context, val);
        return true;
    }

    ~realm() {
        if (_context == nullptr) {
            return;
        }

        tsn_fini(&_context);
    }

private:
    tsn_vm* _context = nullptr;
};

Result<std::string> readFileCached(const fs::path& file) {
    static std::map<std::string, std::string> fileCache;
    auto findItr = fileCache.find(file);
    if (findItr != fileCache.end()) {
        return findItr->second;
    }

    auto loadResult = DiskUtils::load(Path(file.string()));
    if (!loadResult) {
        TEST262_LOG(cerr, "Failed to load {} with error {}", file.string(), loadResult.error().getMessage());
        return loadResult.error();
    }
    const auto loadData = loadResult.moveValue();

    fileCache[file] = loadData.asStringView();
    return fileCache[file];
}

enum class ExecutionResult {
    RunWithPass,
    RunWithError,
    Failed,
};

static ExecutionResult evaluateTestCaseInternal(fs::path harnessDirectory,
                                                std::string testFile,
                                                const TestCaseData& metadata,
                                                bool isStrict,
                                                std::string& error_type,
                                                std::string& error_description) {
    realm realm;

    const auto assertContent = readFileCached(harnessDirectory / kAssertFile);
    const auto staContent = readFileCached(harnessDirectory / kSTAFile);

    if (!assertContent || !staContent) {
        TEST262_LOG(cerr, "Failed to read content of {} or {}", kAssertFile, kSTAFile);
        return ExecutionResult::Failed;
    }

    if (!realm.evaluateHarness(kAssertFile, assertContent.value(), error_type, error_description)) {
        TEST262_LOG(cerr, "Could not process {}", kAssertFile);
        return ExecutionResult::Failed;
    }

    if (!realm.evaluateHarness(kSTAFile, staContent.value(), error_type, error_description)) {
        TEST262_LOG(cerr, "Could not process {}", kSTAFile);
        return ExecutionResult::Failed;
    }

    if (metadata.async) {
        const auto doneprintHandleContent = readFileCached(harnessDirectory / kDonePrintHandle);
        if (!doneprintHandleContent) {
            return ExecutionResult::Failed;
        }

        if (!realm.evaluateHarness(kDonePrintHandle, doneprintHandleContent.value(), error_type, error_description)) {
            TEST262_LOG(cerr, "Could not process {}", kDonePrintHandle);
            return ExecutionResult::Failed;
        }
    }

    for (auto include : metadata.includes) {
        auto include_file = harnessDirectory / include;

        auto includeContents = readFileCached(include_file);
        if (!includeContents) {
            return ExecutionResult::Failed;
        }
        if (!realm.evaluateHarness(include, includeContents.value(), error_type, error_description)) {
            return ExecutionResult::Failed;
        }
    }

    if (!realm.evaluateCompiledTest(testFile, isStrict, metadata.module, error_type, error_description)) {
        return ExecutionResult::RunWithError;
    }
    return ExecutionResult::RunWithPass;
}

static void evaluateTestCase(const std::set<std::string>& skipList,
                             std::string harnessDirectory,
                             std::string testFile,
                             const TestCaseData& metadata,
                             bool isStrict,
                             TestReporter& reporter) {
    const auto lastDelimiterPos = testFile.find_last_of('/');
    const auto testSuite = testFile.substr(0, lastDelimiterPos);
    const auto testName = testFile.substr(lastDelimiterPos + 1, testFile.size());

    if (skipList.contains(testFile)) {
        reporter.submitDisabled(testSuite, testName, "Skipping tests case due to known execution crash", isStrict);
        return;
    }

    if (!tsn_contains_module(testFile.c_str())) {
        reporter.submitDisabled(testSuite, testName, "Skipping tests case due to compilation failure", isStrict);
        return;
    }

    std::string error_type = "";
    std::string error_description = "";

    auto result =
        evaluateTestCaseInternal(harnessDirectory, testFile, metadata, isStrict, error_type, error_description);

    switch (result) {
        case ExecutionResult::RunWithPass: {
            reporter.submitPass(testSuite, testName, isStrict);
        } break;
        case ExecutionResult::RunWithError: {
            if (metadata.isNegative) {
                reporter.submitPass(testSuite, testName, isStrict);
            } else {
                reporter.submitFailure(
                    testSuite,
                    testName,
                    fmt::format("Expected: \"{}\" returned: \"{}\" - ", metadata.negativeType, error_type),
                    isStrict);
            }
        } break;
        case ExecutionResult::Failed: {
            reporter.submitFailure(testSuite, testName, fmt::format("Error: {}", error_type), isStrict);
        } break;
    }
}

bool runTestCases(const fs::path& harnessDirectory,
                  const fs::path& testCaseFile,
                  const fs::path& outputFile,
                  const fs::path& skipListFile) {
    const auto testCasesResult = deserializeTestCases(testCaseFile);
    if (!testCasesResult) {
        TEST262_LOG(cerr, "Failed to parse {} with error {}", testCaseFile, testCasesResult.error().getMessage());
        return false;
    }

    const auto skipList = decodeSkipList(skipListFile);
    const auto testCases = testCasesResult.value();

    TestReporter reporter;

    for (const auto& testCase : testCases) {
        const auto& metadata = testCase.second;
        std::string test_contents;

        // NOTE(rjaber): This should be intentionally omitted by the preprocessor
        if (metadata.isNegative && metadata.negativeType == "SyntaxError") {
            TEST262_LOG(cerr, "Found invalid case in test cases {}", testCase.first);
            return false;
        }

        if (metadata.noStrict || metadata.raw) {
            evaluateTestCase(skipList, harnessDirectory, testCase.first, metadata, false, reporter);
        } else if (metadata.onlyStrict) {
            evaluateTestCase(skipList, harnessDirectory, testCase.first, metadata, true, reporter);
        } else {
            evaluateTestCase(skipList, harnessDirectory, testCase.first, metadata, false, reporter);
            evaluateTestCase(skipList, harnessDirectory, testCase.first, metadata, true, reporter);
        }
    }

    reporter.saveToFile(outputFile);
    return !reporter.hasFailure();
}

int main(int argc, const char* argv[]) {
    fs::path testCaseFile;
    fs::path harnessDirectory;
    fs::path outputFile = std::getenv("XML_OUTPUT_FILE");
    fs::path skipListFile;

    for (auto i = 1; i < argc; i++) {
        string argument(argv[i]);

        if (argument == "-t") {
            testCaseFile = argv[++i];
        } else if (argument == "-h") {
            harnessDirectory = argv[++i];
        } else if (argument == "-s") {
            skipListFile = argv[++i];
        } else {
            TEST262_LOG(cerr, "Unrecognized argument: {}", argument);
            return EXIT_FAILURE;
        }
    }
    harnessDirectory /= "harness";

    if (!fs::exists(skipListFile)) {
        TEST262_LOG(cerr, "Specified skiplist ({}) does not exist", harnessDirectory);
        return EXIT_FAILURE;
    }
    if (!fs::is_directory(harnessDirectory)) {
        TEST262_LOG(cerr, "Specified tests directory({}) is not valid.", harnessDirectory);
        return EXIT_FAILURE;
    }
    if ((fs::exists(outputFile) && fs::is_directory(outputFile)) || outputFile == "") {
        TEST262_LOG(cerr, "Specified output file ({}) exist and is a directory", outputFile);
        return EXIT_FAILURE;
    }
    if (!fs::exists(testCaseFile) || fs::is_directory(testCaseFile)) {
        TEST262_LOG(cerr, "Specified test case file ({}) does not exist or is a directory", testCaseFile);
        return EXIT_FAILURE;
    }

    return runTestCases(harnessDirectory, testCaseFile, outputFile, skipListFile) ? EXIT_SUCCESS : EXIT_FAILURE;
}
