#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class TestReporter {
public:
    TestReporter() = default;
    ~TestReporter() = default;

    void submitPass(const std::string& suite, const std::string& name, bool isStrict);
    void submitDisabled(const std::string& suite,
                        const std::string& name,
                        const std::string& skipReason,
                        bool isStrict);
    void submitFailure(const std::string& suite,
                       const std::string& name,
                       const std::string& failureReason,
                       bool isStrict);

    void saveToFile(const std::filesystem::path& file);

    bool hasFailure();

private:
    enum class TestResult { Pass, Disable, Fail };

    struct TestReport {
        TestResult result;
        std::string message;
    };

    struct TestStatistics {
        int fail = 0;
        int disabled = 0;
        int total = 0;
    };

    void submitReport(const std::string& suite, const std::string& name, const TestReport& report, bool isStrict);

    TestStatistics _aggregateStats;
    std::unordered_map<std::string, TestStatistics> _testSuiteStats;
    std::unordered_map<std::string, std::unordered_map<std::string, TestReport>> _testSuite;
    std::string _content;
};