#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct TestCaseData {
    bool isNegative = false;
    std::string negativePhase;
    std::string negativeType;
    std::vector<std::string> includes;

    bool onlyStrict = false;
    bool noStrict = false;
    bool module = false;
    bool raw = false;
    bool async = false;
};

bool serializeTestCases(const std::filesystem::path& outputFile,
                        const std::unordered_map<std::string, TestCaseData>& testCases);

Valdi::Result<std::map<std::string, TestCaseData>> deserializeTestCases(const std::filesystem::path& inputFile);