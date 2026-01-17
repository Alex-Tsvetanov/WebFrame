#include "TestReporter.hpp"

#include <fstream>
#include <iostream>
#include <test262/shared/Test262Helpers.hpp>

using namespace std;
namespace fs = filesystem;

void TestReporter::submitReport(const std::string& suite,
                                const std::string& name,
                                const TestReport& report,
                                bool isStrict) {
    _testSuite[suite][name + (isStrict ? "__Strict" : "__NonStrict")] = report;
    _aggregateStats.total += 1;
    _testSuiteStats[suite].total += 1;
    switch (report.result) {
        case TestResult::Pass: {
            // NOOP
        } break;
        case TestResult::Fail: {
            _aggregateStats.fail += 1;
            _testSuiteStats[suite].fail += 1;
        } break;
        case TestResult::Disable: {
            _aggregateStats.disabled += 1;
            _testSuiteStats[suite].disabled += 1;
        } break;
    }
}

void TestReporter::submitPass(const std::string& suite, const std::string& name, bool isStrict) {
    const TestReport report = {.result = TestResult::Pass, .message = ""};
    submitReport(suite, name, report, isStrict);
}

void TestReporter::submitDisabled(const std::string& suite,
                                  const std::string& name,
                                  const std::string& skipReason,
                                  bool isStrict) {
    const TestReport report = {.result = TestResult::Disable, .message = skipReason};
    submitReport(suite, name, report, isStrict);
}

void TestReporter::submitFailure(const std::string& suite,
                                 const std::string& name,
                                 const std::string& failureReason,
                                 bool isStrict) {
    const TestReport report = {.result = TestResult::Fail, .message = failureReason};
    submitReport(suite, name, report, isStrict);
}

bool TestReporter::hasFailure() {
    return _aggregateStats.fail > 0;
}

void TestReporter::saveToFile(const fs::path& file) {
    std::fstream outputStream(file, std::ios_base::out | std::ios_base::trunc);
    if (outputStream.fail()) {
        TEST262_LOG(cerr, "Could not create output file {}", file.string());
        return;
    }

    TEST262_LOG(outputStream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    TEST262_LOG(outputStream,
                "<testsuites tests=\"{}\" failures=\"{}\" skipped=\"{}\" errors=\"0\" name=\"AllTests\">",
                _aggregateStats.total,
                _aggregateStats.fail,
                _aggregateStats.disabled);
    for (const auto& suite : _testSuite) {
        const auto stats = _testSuiteStats[suite.first];
        TEST262_LOG(outputStream,
                    "\t<testsuite name=\"{}\" tests=\"{}\" failures=\"{}\" disabled=\"{}\" errors=\"0\">",
                    suite.first,
                    stats.total,
                    stats.fail,
                    stats.disabled);
        for (const auto& testReport : suite.second) {
            // status="notrun" result="suppressed"
            TEST262_LOG(outputStream,
                        "\t\t<testcase name=\"{}\" status=\"{}\" result=\"{}\">",
                        testReport.first,
                        testReport.second.result == TestResult::Disable ? "notrun" : "run",
                        testReport.second.result == TestResult::Disable ? "suppressed" : "completed");
            switch (testReport.second.result) {
                case TestResult::Fail: {
                    TEST262_LOG(outputStream, "\t\t\t<failure></failure>");
                } break;
                case TestResult::Disable: {
                    TEST262_LOG(outputStream, "\t\t\t<disabled></disabled>");
                } break;
                default:
                    break;
            }
            TEST262_LOG(outputStream, "\t\t</testcase>");
        }
        TEST262_LOG(outputStream, "\t</testsuite>");
    }
    TEST262_LOG(outputStream, "</testsuites>");
}