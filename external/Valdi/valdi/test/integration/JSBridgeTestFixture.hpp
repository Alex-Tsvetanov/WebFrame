#pragma once
#include "valdi/jsbridge/JavaScriptBridge.hpp"
#include "gtest/gtest.h"
#include <ostream>
#include <string>

namespace ValdiTest {

enum class JavaScriptEngineTestCase { JSCore, Hermes, V8, QuickJS, QuickJSWithTSN };

struct PrintJavaScriptEngineType {
    std::string operator()(const testing::TestParamInfo<JavaScriptEngineTestCase>& info) const;
};

class JSBridgeTestFixture : public ::testing::TestWithParam<JavaScriptEngineTestCase> {
public:
    Valdi::IJavaScriptBridge* getJsBridge() const;

    bool isHermes() const;
    bool isJSCore() const;
    bool isV8() const;
    bool isQuickJS() const;

    bool isWithTSN() const;
};

}; // namespace ValdiTest
