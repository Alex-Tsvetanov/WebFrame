#include "Test262Serialization.hpp"
#include "Test262Helpers.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

using namespace std;
namespace fs = filesystem;
using namespace Valdi;

static const StringBox kIsNegative = STRING_LITERAL("isNegative");
static const StringBox kNegativePhase = STRING_LITERAL("negativePhase");
static const StringBox kNegativeType = STRING_LITERAL("negativeType");
static const StringBox kIncludes = STRING_LITERAL("includes");
static const StringBox kOnlyStrict = STRING_LITERAL("onlyStrict");
static const StringBox kNoStrict = STRING_LITERAL("noStrict");
static const StringBox kModule = STRING_LITERAL("module");
static const StringBox kRaw = STRING_LITERAL("raw");
static const StringBox kAsync = STRING_LITERAL("async");

// MUST(rjaber): Add recursive iteration, and relative path into DiskUtils/Path and move this function over
bool serializeTestCases(const fs::path& outputFile, const unordered_map<string, TestCaseData>& testCases) {
    Value result;
    for (const auto& testCase : testCases) {
        auto data = testCase.second;

        Value map;
        map.setMapValue(kIsNegative, Value(data.isNegative));
        map.setMapValue(kNegativePhase, Value(data.negativePhase));
        map.setMapValue(kNegativeType, Value(data.negativeType));
        map.setMapValue(kOnlyStrict, Value(data.onlyStrict));
        map.setMapValue(kNoStrict, Value(data.noStrict));
        map.setMapValue(kModule, Value(data.module));
        map.setMapValue(kRaw, Value(data.raw));
        map.setMapValue(kAsync, Value(data.async));

        const auto includesSize = data.includes.size();
        auto includesValueArray = ValueArray::make(includesSize);
        for (size_t i = 0; i < includesSize; i++) {
            includesValueArray->emplace(i, Value(data.includes[i]));
        }
        map.setMapValue(kIncludes, Value(includesValueArray));
        result.setMapValue(testCase.first, map);
    }

    const auto data = valueToJson(result)->toBytesView();

    auto storeResult = DiskUtils::store(Path(outputFile.string()), data);
    if (!storeResult) {
        TEST262_LOG(cerr, "Failed to save file {} with error {}", outputFile, storeResult.error().getMessage());
        return false;
    }
    return true;
}

Result<map<string, TestCaseData>> deserializeTestCases(const fs::path& inputFile) {
    auto testCasesResult = DiskUtils::load(Path(inputFile.string()));
    if (!testCasesResult) {
        return TEST262_ERROR("Failed to save file {} with error {}", inputFile, testCasesResult.error().getMessage());
    }
    const auto testCasesData = testCasesResult.moveValue();
    auto testCasesValueResult = jsonToValue(testCasesData.data(), testCasesResult.value().size());
    if (!testCasesValueResult) {
        return TEST262_ERROR(
            "Failed to parse file {} with error {}", inputFile, testCasesValueResult.error().getMessage());
    }
    const auto testCasesValue = testCasesValueResult.moveValue();
    if (!testCasesValue.isMap()) {
        return TEST262_ERROR("Parsed file {} expected top level map", inputFile);
    }

    map<string, TestCaseData> retval;
    for (const auto& testCase : *testCasesValue.getMapRef()) {
        TestCaseData data;
        if (!testCase.second.isMap()) {
            return TEST262_ERROR("Test case data for {} is not a map", testCase.first);
        }

        const auto isNegativeValue = testCase.second.getMapValue(kIsNegative);
        const auto negativePhaseValue = testCase.second.getMapValue(kNegativePhase);
        const auto negativeTypeValue = testCase.second.getMapValue(kNegativeType);
        const auto onlyStrictValue = testCase.second.getMapValue(kOnlyStrict);
        const auto noStrictValue = testCase.second.getMapValue(kNoStrict);
        const auto moduleValue = testCase.second.getMapValue(kModule);
        const auto rawValue = testCase.second.getMapValue(kRaw);
        const auto asyncValue = testCase.second.getMapValue(kAsync);
        const auto includesValue = testCase.second.getMapValue(kIncludes);

        bool boolValueCheck = isNegativeValue.isBool() && onlyStrictValue.isBool() && noStrictValue.isBool() &&
                              moduleValue.isBool() && rawValue.isBool() && asyncValue.isBool();
        if (!boolValueCheck) {
            return TEST262_ERROR("Test case data for {} expected bool type", testCase.first);
        }
        bool stringValueCheck = negativePhaseValue.isString() && negativeTypeValue.isString();
        if (!stringValueCheck) {
            return TEST262_ERROR("Test case data for {} expected string type", testCase.first);
        }
        bool arrayValueCheck = includesValue.isArray();
        if (!arrayValueCheck) {
            return TEST262_ERROR("Test case data for {} expected array type", testCase.first);
        }

        data.isNegative = isNegativeValue.toBool();
        data.onlyStrict = onlyStrictValue.toBool();
        data.noStrict = noStrictValue.toBool();
        data.module = moduleValue.toBool();
        data.async = asyncValue.toBool();

        data.negativePhase = negativePhaseValue.toString();
        data.negativeType = negativeTypeValue.toString();

        data.includes.reserve(includesValue.getArrayRef()->size());
        for (const auto& element : *includesValue.getArrayRef()) {
            if (!element.isString()) {
                return TEST262_ERROR("Test case data for {} expected string type check failed", testCase.first);
            }
            data.includes.push_back(element.toString());
        }

        retval[testCase.first.slowToString()] = data;
    }

    return retval;
}
