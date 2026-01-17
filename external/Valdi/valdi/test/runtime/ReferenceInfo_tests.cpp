#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(ReferenceInfo, supportsEmptyState) {
    auto ref = ReferenceInfoBuilder().build();

    ASSERT_EQ("<unknown>", ref.toString());
}

TEST(ReferenceInfo, supportsObject) {
    auto myObjectKey = STRING_LITERAL("myObject");
    auto ref = ReferenceInfoBuilder().withObject(myObjectKey).build();

    ASSERT_EQ("myObject", ref.toString());
}

TEST(ReferenceInfo, supportsObjectProperty) {
    auto myObjectKey = STRING_LITERAL("myObject");
    auto propKey = STRING_LITERAL("prop");
    auto ref = ReferenceInfoBuilder().withObject(myObjectKey).withProperty(propKey).build();

    ASSERT_EQ("myObject.prop", ref.toString());
}

TEST(ReferenceInfo, canChainProperties) {
    auto myObjectKey = STRING_LITERAL("myObject");
    auto propAKey = STRING_LITERAL("propA");
    auto propBKey = STRING_LITERAL("propB");
    auto ref = ReferenceInfoBuilder().withObject(myObjectKey).withProperty(propAKey).withProperty(propBKey).build();

    ASSERT_EQ("<unknown_property>.propA.propB", ref.toString());
}

TEST(ReferenceInfo, supportsArrayIndex) {
    auto myObjectKey = STRING_LITERAL("myObject");
    auto propAKey = STRING_LITERAL("propA");
    auto ref = ReferenceInfoBuilder().withObject(myObjectKey).withProperty(propAKey).withArrayIndex(4).build();

    ASSERT_EQ("myObject.propA[4]", ref.toString());
}

TEST(ReferenceInfo, supportsFunction) {
    auto moduleKey = STRING_LITERAL("Module");
    auto callbackKey = STRING_LITERAL("callback");

    auto ref = ReferenceInfoBuilder().withObject(moduleKey).withProperty(callbackKey).asFunction().build();

    ASSERT_EQ("Module.callback()", ref.toString());
}

TEST(ReferenceInfo, supportsFunctionParameter) {
    auto moduleKey = STRING_LITERAL("Module");
    auto callbackKey = STRING_LITERAL("callback");
    auto ref =
        ReferenceInfoBuilder().withObject(moduleKey).withProperty(callbackKey).asFunction().withParameter(2).build();

    ASSERT_EQ("Module.callback() -> <parameter 2>", ref.toString());
}

TEST(ReferenceInfo, supportsFunctionReturnValue) {
    auto moduleKey = STRING_LITERAL("Module");
    auto callbackKey = STRING_LITERAL("callback");
    auto ref =
        ReferenceInfoBuilder().withObject(moduleKey).withProperty(callbackKey).asFunction().withReturnValue().build();

    ASSERT_EQ("Module.callback() -> <return_value>", ref.toString());
}

TEST(ReferenceInfo, canBuildUponPreviouslyCreatedReference) {
    auto myFunctionKey = STRING_LITERAL("myFunction");
    auto ref = ReferenceInfoBuilder().withProperty(myFunctionKey).asFunction().withParameter(2).build();

    ASSERT_EQ("myFunction() -> <parameter 2>", ref.toString());

    auto newRef = ReferenceInfoBuilder(ref).withArrayIndex(0).asFunction().withParameter(1).asFunction().build();

    ASSERT_EQ("myFunction() -> <parameter 2>[0]() -> <parameter 1>()", newRef.toString());

    auto newRef2 = ReferenceInfoBuilder(newRef).withArrayIndex(4).build();

    ASSERT_EQ("<parameter 2>[0]() -> <parameter 1>()[4]", newRef2.toString());
}

TEST(ReferenceInfo, loosesInformationAfterLongChaining) {
    auto moduleKey = STRING_LITERAL("Module");
    auto callbackKey = STRING_LITERAL("callback");
    auto resultKey = STRING_LITERAL("result");
    auto codeKey = STRING_LITERAL("code");

    auto ref = ReferenceInfoBuilder()
                   .withObject(moduleKey)
                   .withProperty(callbackKey)
                   .asFunction()
                   .withReturnValue()
                   .withProperty(resultKey)
                   .build();

    ASSERT_EQ("<unknown_property>.callback() -> <return_value>.result", ref.toString());

    ref = ReferenceInfoBuilder(ref).withProperty(codeKey).build();

    ASSERT_EQ("<unknown_function>() -> <return_value>.result.code", ref.toString());

    ref = ReferenceInfoBuilder(ref).asFunction().withReturnValue().build();

    ASSERT_EQ("<return_value>.result.code() -> <return_value>", ref.toString());
}

TEST(ReferenceInfo, canGenerateFunctionIdentifier) {
    auto moduleKey = STRING_LITERAL("Module");
    auto callbackKey = STRING_LITERAL("callback");
    auto resultKey = STRING_LITERAL("result");
    auto codeKey = STRING_LITERAL("code");

    auto ref = ReferenceInfoBuilder().withObject(callbackKey).build();

    ASSERT_EQ(STRING_LITERAL("callback"), ref.toFunctionIdentifier());

    auto ref2 = ReferenceInfoBuilder().withObject(callbackKey).withParameter(1).build();

    ASSERT_EQ(STRING_LITERAL("callback.<p1>"), ref2.toFunctionIdentifier());

    auto ref3 = ReferenceInfoBuilder()
                    .withObject(moduleKey)
                    .withProperty(callbackKey)
                    .asFunction()
                    .withReturnValue()
                    .withProperty(resultKey)
                    .build();

    ASSERT_EQ(STRING_LITERAL("<anon>.callback.<r>.result"), ref3.toFunctionIdentifier());
}

} // namespace ValdiTest
