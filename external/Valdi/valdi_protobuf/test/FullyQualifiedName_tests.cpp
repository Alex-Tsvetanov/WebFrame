#include "valdi_protobuf/FullyQualifiedName.hpp"
#include "gtest/gtest.h"

#include <algorithm>
#include <google/protobuf/descriptor.pb.h>

using namespace Valdi;
namespace {

TEST(FullyQualifiedName, canInitWithSinglePackage) {
    Protobuf::FullyQualifiedName name("com");

    ASSERT_EQ("", name.getLeadingComponents());
    ASSERT_EQ("com", name.getLastComponent());
}

TEST(FullyQualifiedName, canInitWithNestedPackage) {
    Protobuf::FullyQualifiedName name("com.valdi");

    ASSERT_EQ("com", name.getLeadingComponents());
    ASSERT_EQ("valdi", name.getLastComponent());

    name = Protobuf::FullyQualifiedName("com.valdi.MyType");

    ASSERT_EQ("com.valdi", name.getLeadingComponents());
    ASSERT_EQ("MyType", name.getLastComponent());
}

TEST(FullyQualifiedName, canAppend) {
    Protobuf::FullyQualifiedName name;

    ASSERT_EQ("", name.getLeadingComponents());
    ASSERT_EQ("", name.getLastComponent());

    name.append("com");

    ASSERT_EQ("", name.getLeadingComponents());
    ASSERT_EQ("com", name.getLastComponent());

    name.append("valdi");

    ASSERT_EQ("com", name.getLeadingComponents());
    ASSERT_EQ("valdi", name.getLastComponent());

    name.append("nested.MyType");

    ASSERT_EQ("com.valdi.nested", name.getLeadingComponents());
    ASSERT_EQ("MyType", name.getLastComponent());
}

TEST(FullyQualifiedName, canRemoveLastComponent) {
    Protobuf::FullyQualifiedName name("com.valdi.MyType");

    ASSERT_EQ("com.valdi", name.getLeadingComponents());
    ASSERT_EQ("MyType", name.getLastComponent());

    ASSERT_TRUE(name.canRemoveLastComponent());
    name = name.removingLastComponent();

    ASSERT_EQ("com", name.getLeadingComponents());
    ASSERT_EQ("valdi", name.getLastComponent());

    ASSERT_TRUE(name.canRemoveLastComponent());
    name = name.removingLastComponent();

    ASSERT_EQ("", name.getLeadingComponents());
    ASSERT_EQ("com", name.getLastComponent());

    ASSERT_FALSE(name.canRemoveLastComponent());
}

TEST(FullyQualifiedName, canOutputAsString) {
    Protobuf::FullyQualifiedName name("com.valdi.MyType");

    ASSERT_EQ("com.valdi.MyType", name.toString());

    name = Protobuf::FullyQualifiedName("MyType");

    ASSERT_EQ("MyType", name.toString());
}

} // namespace
