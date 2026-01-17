#include "test262/shared/Test262Helpers.hpp"
#include "test262/shared/Test262Serialization.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;
namespace fs = filesystem;

static TestCaseData parseMetaDataComment(string& contents) {
    TestCaseData retval;

    auto state = 0;

    string attribute;
    string key;
    string entry;

    // NOTE(rjaber): This parser isn't great but mostly kept it going from the original test harness that we got
    for (auto c : contents) {
        switch (state) {
            case 0:
                if (c == '\r' || c == '\n') {
                    state = 1;
                }

                break;

            case 1:
                if (c <= ' ') {
                    if (c != '\r' && c != '\n') {
                        state = 0;
                    }

                    break;
                }

                attribute.clear();

                attribute.push_back(c);

                state = 2;

                break;

            case 2:
                if (c == ':') {
                    if (attribute == "negative") {
                        retval.isNegative = true;

                        state = 3;
                    } else if (attribute == "includes") {
                        state = 6;
                    } else if (attribute == "flags") {
                        state = 13;
                    } else {
                        state = 0;
                    }

                    break;
                }

                attribute.push_back(c);

                break;

            case 3:
                if (c == ':') {
                    if (key == "phase") {
                        state = 4;
                    } else if (key == "type") {
                        state = 5;
                    }

                    break;
                } else if (c <= ' ') {
                    break;
                }

                key.push_back(c);

                break;

            case 4:
                if (c <= ' ') {
                    if (retval.negativePhase == "") {
                        break;
                    }

                    if (retval.negativeType == "") {
                        key.clear();

                        state = 3;

                        break;
                    }

                    state = 0;

                    break;
                }

                retval.negativePhase.push_back(c);
                break;
            case 5:
                if (c <= ' ') {
                    if (retval.negativeType == "") {
                        break;
                    }

                    if (retval.negativeType == "") {
                        key.clear();

                        state = 3;

                        break;
                    }

                    state = 0;

                    break;
                }

                retval.negativeType.push_back(c);

                break;

            case 6:
                if (c == '[') {
                    entry.clear();

                    state = 7;
                } else if (c == '-') {
                    state = 8;
                }

                break;

            case 7:
                if (c == ']' || c == ',') {
                    if (entry != "") {
                        retval.includes.push_back(entry);

                        entry.clear();
                    }

                    if (c == ']') {
                        attribute.clear();

                        state = 0;
                    }

                    break;
                } else if (c <= ' ') {
                    break;
                }

                entry.push_back(c);

                break;

            case 8:
                if (c == ' ') {
                    entry.clear();

                    state = 9;
                } else {
                    state = 0;
                }

                break;

            case 9:
                if (c <= ' ') {
                    if (entry == "") {
                        break;
                    }

                    retval.includes.push_back(entry);

                    entry.clear();

                    if (c == '\r' || c == '\n') {
                        state = 11;
                    } else {
                        state = 10;
                    }

                    break;
                }

                entry.push_back(c);

                break;

            case 10:
                if (c == '\r' || c == '\n') {
                    state = 11;
                }

                break;

            case 11:
                if (c <= ' ') {
                    if (c != '\r' && c != '\n') {
                        state = 12;
                    }

                    break;
                }

                attribute.clear();

                attribute.push_back(c);

                state = 2;

                break;

            case 12:
                if (c == '-') {
                    state = 8;
                }

                break;

            case 13:
                if (c == '[') {
                    entry.clear();

                    state = 14;
                }

                break;

            case 14:
                if (c == ']' || c == ',') {
                    if (entry == "onlyStrict") {
                        retval.onlyStrict = true;
                    } else if (entry == "noStrict") {
                        retval.noStrict = true;
                    } else if (entry == "module") {
                        retval.module = true;
                    } else if (entry == "raw") {
                        retval.raw = true;
                    } else if (entry == "async") {
                        retval.async = true;
                    }

                    if (c == ']') {
                        state = 0;
                    }

                    break;
                } else if (c <= ' ') {
                    break;
                }

                entry.push_back(c);

                break;
        }
    }

    return retval;
}

static Valdi::Result<TestCaseData> parseMetaData(const string& content) {
    const string metadata_start("/*---");
    const string metadata_end("---*/");

    auto metadata_start_position = content.find(metadata_start);
    if (metadata_start_position != string::npos) {
        auto metadata_end_position = content.find(metadata_end);

        if (metadata_end_position != string::npos) {
            auto metadata_contents =
                content.substr(metadata_start_position + metadata_start.length(),
                               metadata_end_position - metadata_start_position - metadata_start.length());

            return parseMetaDataComment(metadata_contents);
        }
    }
    return {};
}

bool processTestDirectory(const fs::path& test262Directory, const fs::path& outputFile) {
    auto testDirectory = test262Directory / "test";
    auto tmp = string(testDirectory.c_str());

    unordered_map<std::string, TestCaseData> testCases;
    // TODO(rjaber): Add support for skipping folders, for example we don't expect Intl tests to ever pass
    // TODO(rjaber): Add directory_entry into DiskUtils, and move away from using C++ filesystem module
    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(testDirectory)) {
        if (dir_entry.is_directory()) {
            continue;
        }

        auto path = dir_entry.path();
        if (path.extension() != ".js" || path.filename().string().ends_with("_FIXTURE.js")) {
            continue;
        }

        auto contentResult = Valdi::DiskUtils::load(Valdi::Path(path.string()));
        if (!contentResult) {
            TEST262_LOG(cerr, "Failed to load file {} with error {}", path, contentResult.error().getMessage());
            return false;
        }
        auto content = contentResult.moveValue();
        auto metadataResult = parseMetaData(string(content.asStringView()));
        if (!metadataResult) {
            TEST262_LOG(cerr, "Could not process metadata for file {}", path);
            return false;
        }
        auto metadata = metadataResult.moveValue();

        // NOTE(rjaber): For the tests cases these checks are not important since the typescript VM should be taking
        // care of it.
        if (metadata.isNegative && metadata.negativeType == "SyntaxError") {
            continue;
        }

        // TODO(rjaber): Add relative into Valdi::Path, and move away from using c++ filesystem module
        const auto directory = fs::path(test262Directory).parent_path();
        const auto testFileRelative = fs::relative(path, directory).replace_extension("");
        testCases[testFileRelative.string()] = metadata;
    }

    serializeTestCases(outputFile, testCases);
    return true;
}

int main(int argc, const char* argv[]) {
    fs::path testDirectory;
    fs::path outputFile;

    for (auto i = 1; i < argc; i++) {
        string argument(argv[i]);

        if (argument == "-t") {
            testDirectory = argv[++i];
        } else if (argument == "-o") {
            outputFile = argv[++i];
        } else {
            TEST262_LOG(cerr, "Unrecognized argument: {}", argument);
            return EXIT_FAILURE;
        }
    }

    if (!fs::is_directory(testDirectory)) {
        TEST262_LOG(cerr, "Specified tests directory({}) is not valid.", testDirectory);
        return EXIT_FAILURE;
    }
    if (fs::exists(outputFile) && fs::is_directory(outputFile)) {
        TEST262_LOG(cerr, "Specified output file ({}) exist and is a directory", outputFile);
        return EXIT_FAILURE;
    }

    processTestDirectory(testDirectory, outputFile);

    return EXIT_SUCCESS;
}
