//
//  ValueUtil_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 11/7/19.
//

#include "valdi/runtime/Attributes/ValueConverters.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueArray.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

enum class JSONParseMode { PREFER_CORRECTNESS = 0, PREFER_PERFORMANCE = 1 };

static Result<Value> jsonToValue(const std::string_view& str, JSONParseMode parseMode) {
    switch (parseMode) {
        case JSONParseMode::PREFER_CORRECTNESS:
            return correctJsonToValue(str);
        case JSONParseMode::PREFER_PERFORMANCE:
            return fastJsonToValue(str);
    }
}

class ValueUtilsFixture : public ::testing::TestWithParam<JSONParseMode> {
public:
    JSONParseMode getParseMode() {
        return GetParam();
    }
};

TEST(ValueUtils, encodePrimitiveTypesToJson) {
    Value map1 = Value().setMapValue("bool", Value(true));
    ASSERT_EQ(STRING_LITERAL("{\"bool\":true}").toStringView(), valueToJson(map1)->toBytesView().asStringView());

    Value map2 = Value().setMapValue("int", Value(23));
    ASSERT_EQ(STRING_LITERAL("{\"int\":23}").toStringView(), valueToJson(map2)->toBytesView().asStringView());

    Value map3 = Value().setMapValue("long", Value(2147483646));
    ASSERT_EQ(STRING_LITERAL("{\"long\":2147483646}").toStringView(), valueToJson(map3)->toBytesView().asStringView());

    Value map4 = Value().setMapValue("double", Value(12.5));
    ASSERT_EQ(STRING_LITERAL("{\"double\":12.500000}").toStringView(), valueToJson(map4)->toBytesView().asStringView());
}

TEST_P(ValueUtilsFixture, decodePrimitiveTypesToValue) {
    Value map1 = Value().setMapValue("bool", Value(true));
    ASSERT_EQ(map1, jsonToValue(STRING_LITERAL("{\"bool\":true}").toStringView(), getParseMode()).value());

    Value map2 = Value().setMapValue("int", Value(23));
    ASSERT_EQ(map2, jsonToValue(STRING_LITERAL("{\"int\":23}").toStringView(), getParseMode()).value());

    Value map3 = Value().setMapValue("long", Value(2147483646));
    ASSERT_EQ(map3, jsonToValue(STRING_LITERAL("{\"long\":2147483646}").toStringView(), getParseMode()).value());

    Value map4 = Value().setMapValue("double", Value(12.5));
    ASSERT_EQ(map4, jsonToValue(STRING_LITERAL("{\"double\":12.500000}").toStringView(), getParseMode()).value());
}

TEST(ValueUtils, encodeSimpleStringToJson) {
    auto quoteString = "Things and \"stuff\"";
    Value quotes = Value().setMapValue("quote", Value(quoteString));
    ASSERT_EQ(STRING_LITERAL("{\"quote\":\"Things and \\\"stuff\\\"\"}").toStringView(),
              valueToJson(quotes)->toBytesView().asStringView());

    auto slashString = "Things and \\stuff\\";
    Value slash = Value().setMapValue("slash", Value(slashString));
    ASSERT_EQ(STRING_LITERAL("{\"slash\":\"Things and \\\\stuff\\\\\"}").toStringView(),
              valueToJson(slash)->toBytesView().asStringView());
}

TEST_P(ValueUtilsFixture, deodeSimpleStringToValue) {
    auto quoteString = "Things and \"stuff\"";
    Value quotes = Value().setMapValue("quote", Value(quoteString));
    ASSERT_EQ(
        quotes,
        jsonToValue(STRING_LITERAL("{\"quote\":\"Things and \\\"stuff\\\"\"}").toStringView(), getParseMode()).value());

    auto slashString = "Things and \\stuff\\";
    Value slash = Value().setMapValue("slash", Value(slashString));
    ASSERT_EQ(
        slash,
        jsonToValue(STRING_LITERAL("{\"slash\":\"Things and \\\\stuff\\\\\"}").toStringView(), getParseMode()).value());
}

TEST(ValueUtils, encodeCtrlCharactersStringToJson) {
    auto tabString = "\t";
    Value tab = Value().setMapValue("string", Value(tabString));
    ASSERT_EQ(STRING_LITERAL("{\"string\":\"\\t\"}").toStringView(), valueToJson(tab)->toBytesView().asStringView());

    auto newlineString = "\n";
    Value newline = Value().setMapValue("string", Value(newlineString));
    ASSERT_EQ(STRING_LITERAL("{\"string\":\"\\n\"}").toStringView(),
              valueToJson(newline)->toBytesView().asStringView());

    auto vTabString = "\v";
    Value vTab = Value().setMapValue("string", Value(vTabString));
    ASSERT_EQ(STRING_LITERAL("{\"string\":\"\\v\"}").toStringView(), valueToJson(vTab)->toBytesView().asStringView());

    auto formFeedString = "\f";
    Value formFeed = Value().setMapValue("string", Value(formFeedString));
    ASSERT_EQ(STRING_LITERAL("{\"string\":\"\\f\"}").toStringView(),
              valueToJson(formFeed)->toBytesView().asStringView());

    auto carriageReturnString = "\r";
    Value carriageReturn = Value().setMapValue("string", Value(carriageReturnString));
    ASSERT_EQ(STRING_LITERAL("{\"string\":\"\\r\"}").toStringView(),
              valueToJson(carriageReturn)->toBytesView().asStringView());

    auto allString = "\t\n\v\f\r";
    Value all = Value().setMapValue("string", Value(allString));
    ASSERT_EQ(STRING_LITERAL("{\"string\":\"\\t\\n\\v\\f\\r\"}").toStringView(),
              valueToJson(all)->toBytesView().asStringView());
}

TEST(ValueUtils, encodeUnicodeAsHex) {
    auto tabString = "Hey 😉, nice!";
    Value tab = Value().setMapValue("string", Value(tabString));
    // OS-specific variation: jsoncpp version is different between macOS (1.8.0)
    // and linux (1.9.6) where 1.9.6 machines default to UTF-16 and earlier versions
    // default to UTF-8. Allow both so the test passes regardless of version.
    EXPECT_THAT(valueToJson(tab)->toBytesView().asStringView(),
                ::testing::AnyOf(STRING_LITERAL("{\"string\":\"Hey \\ud83d\\ude09, nice!\"}").toStringView(),
                                 STRING_LITERAL("{\"string\":\"Hey \xF0\x9F\x98\x89, nice!\"}").toStringView()));
}

TEST_P(ValueUtilsFixture, decodeHexUnicode) {
    auto tabString = "Hey 😉, nice!";
    Value tab = Value().setMapValue("string", Value(tabString));
    ASSERT_EQ(tab,
              jsonToValue(STRING_LITERAL("{\"string\":\"Hey \\ud83d\\ude09, nice!\"}").toStringView(), getParseMode())
                  .value());
}

TEST_P(ValueUtilsFixture, decodeCtrlCharactersToValue) {
    auto tabString = "\t";
    Value tab = Value().setMapValue("string", Value(tabString));
    ASSERT_EQ(tab, jsonToValue(STRING_LITERAL("{\"string\":\"\\t\"}").toStringView(), getParseMode()).value());

    auto newlineString = "\n";
    Value newline = Value().setMapValue("string", Value(newlineString));
    ASSERT_EQ(newline, jsonToValue(STRING_LITERAL("{\"string\":\"\\n\"}").toStringView(), getParseMode()).value());

    auto formFeedString = "\f";
    Value formFeed = Value().setMapValue("string", Value(formFeedString));
    ASSERT_EQ(formFeed, jsonToValue(STRING_LITERAL("{\"string\":\"\\f\"}").toStringView(), getParseMode()).value());

    auto carriageReturnString = "\r";
    Value carriageReturn = Value().setMapValue("string", Value(carriageReturnString));
    ASSERT_EQ(carriageReturn,
              jsonToValue(STRING_LITERAL("{\"string\":\"\\r\"}").toStringView(), getParseMode()).value());

    auto allString = "\t\n\f\r";
    Value all = Value().setMapValue("string", Value(allString));
    ASSERT_EQ(all, jsonToValue(STRING_LITERAL("{\"string\":\"\\t\\n\\f\\r\"}").toStringView(), getParseMode()).value());
}

INSTANTIATE_TEST_SUITE_P(ValueUtilsTests,
                         ValueUtilsFixture,
                         ::testing::Values(JSONParseMode::PREFER_CORRECTNESS, JSONParseMode::PREFER_PERFORMANCE));

} // namespace ValdiTest
