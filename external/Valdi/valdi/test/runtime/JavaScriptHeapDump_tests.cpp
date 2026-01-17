//
//  JavaScriptHeapDump_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 20/12/23
//

#include "valdi/runtime/JavaScript/JavaScriptHeapDumpBuilder.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include <gtest/gtest.h>

#define ASSERT_NO_EXCEPTION(__exceptionTracker__)                                                                      \
    if (!__exceptionTracker__) {                                                                                       \
        ASSERT_FALSE(true) << __exceptionTracker__.extractError().toString();                                          \
        __exceptionTracker__.clearError();                                                                             \
        return;                                                                                                        \
    }

using namespace Valdi;

namespace ValdiTest {

TEST(JavaScriptHeapDumpParser, canParseHeapDump) {
    std::string_view json =
        R"({"snapshot":{"meta":{"node_fields":["type","name","id","self_size","edge_count","trace_node_id","detachedness"],"node_types":[["hidden","array","string","object","code","closure","regexp","number","native","synthetic","symbol","bigint","object shape"],"string","number","number","number","number","number"],"edge_fields":["type","name_or_index","to_node"],"edge_types":[["context","element","property","internal","hidden","shortcut","weak"],"string_or_number","node"]},"node_count":3,"edge_count":2,"trace_function_count":0},
"nodes":[3,0,1,40,1,0,0,
1,2,2,100,1,0,0,
2,3,3,11,0,0,0],
"edges":[2,1,7,
1,0,14],
"trace_function_infos":[],
"trace_tree":[],
"samples":[],
"locations":[],
"strings":[
"Object",
"items",
"Array",
"Hello World"]}
)";

    SimpleExceptionTracker exceptionTracker;
    JavaScriptHeapDumpParser parser(reinterpret_cast<const Byte*>(json.data()), json.size(), exceptionTracker);

    ASSERT_NO_EXCEPTION(exceptionTracker);

    auto nodes = parser.getNodes();

    ASSERT_EQ(static_cast<size_t>(3), nodes.size());

    ASSERT_EQ(JavaScriptHeapDumpNodeType::OBJECT, nodes[0].getType());
    ASSERT_EQ(STRING_LITERAL("Object"), nodes[0].getName());
    ASSERT_EQ(static_cast<uint64_t>(1), nodes[0].getId());
    ASSERT_EQ(static_cast<size_t>(40), nodes[0].getSelfSizeBytes());

    auto edges1 = nodes[0].getEdges();
    ASSERT_EQ(static_cast<size_t>(1), edges1.size());

    ASSERT_EQ(JavaScriptHeapDumpEdgeType::PROPERTY, edges1[0].getType());
    ASSERT_EQ(JavaScriptHeapEdgeIdentifier::named(STRING_LITERAL("items")), edges1[0].getIdentifier());
    ASSERT_EQ(static_cast<uint64_t>(2), edges1[0].getNode().getId());

    ASSERT_EQ(JavaScriptHeapDumpNodeType::ARRAY, nodes[1].getType());
    ASSERT_EQ(STRING_LITERAL("Array"), nodes[1].getName());
    ASSERT_EQ(static_cast<uint64_t>(2), nodes[1].getId());
    ASSERT_EQ(static_cast<size_t>(100), nodes[1].getSelfSizeBytes());

    auto edges2 = nodes[1].getEdges();
    ASSERT_EQ(static_cast<size_t>(1), edges2.size());

    ASSERT_EQ(JavaScriptHeapDumpEdgeType::ELEMENT, edges2[0].getType());
    ASSERT_EQ(JavaScriptHeapEdgeIdentifier::indexed(0), edges2[0].getIdentifier());
    ASSERT_EQ(static_cast<uint64_t>(3), edges2[0].getNode().getId());

    ASSERT_EQ(JavaScriptHeapDumpNodeType::STRING, nodes[2].getType());
    ASSERT_EQ(STRING_LITERAL("Hello World"), nodes[2].getName());
    ASSERT_EQ(static_cast<uint64_t>(3), nodes[2].getId());

    auto edges3 = nodes[2].getEdges();

    ASSERT_EQ(static_cast<size_t>(0), edges3.size());
}

TEST(JavaScriptHeapDumpBuilder, canBuildHeapDump) {
    JavaScriptHeapDumpBuilder builder;

    builder.beginNode(JavaScriptHeapDumpNodeType::OBJECT, STRING_LITERAL("Object"), 1, 40);
    builder.appendEdge(
        JavaScriptHeapDumpEdgeType::PROPERTY, JavaScriptHeapEdgeIdentifier::named(STRING_LITERAL("items")), 2);
    builder.beginNode(JavaScriptHeapDumpNodeType::ARRAY, STRING_LITERAL("Array"), 2, 100);
    builder.appendEdgeToValue(JavaScriptHeapDumpEdgeType::ELEMENT,
                              JavaScriptHeapEdgeIdentifier::indexed(0),
                              JavaScriptHeapDumpNodeType::STRING,
                              3,
                              "Hello World",
                              11);

    auto dump = builder.build();

    std::string str = std::string(reinterpret_cast<const char*>(dump.data()), dump.size());

    auto result = jsonToValue(dump.data(), dump.size());

    ASSERT_TRUE(result) << result.description();

    std::string_view expectedJSON =
        R"({"snapshot":{"meta":{"node_fields":["type","name","id","self_size","edge_count","trace_node_id","detachedness"],"node_types":[["hidden","array","string","object","code","closure","regexp","number","native","synthetic","symbol","bigint","object shape"],"string","number","number","number","number","number"],"edge_fields":["type","name_or_index","to_node"],"edge_types":[["context","element","property","internal","hidden","shortcut","weak"],"string_or_number","node"]},"node_count":3,"edge_count":2,"trace_function_count":0},
"nodes":[3,0,1,40,1,0,0,
1,2,2,100,1,0,0,
2,3,3,11,0,0,0],
"edges":[2,1,7,
1,0,14],
"strings":[
"Object",
"items",
"Array",
"Hello World"]}
)";

    auto expectedJSONValue = jsonToValue(expectedJSON);
    ASSERT_TRUE(expectedJSONValue) << expectedJSONValue.description();

    ASSERT_EQ(expectedJSONValue.value(), result.value());
}

TEST(JavaScriptHeapDumpBuilder, canMergeHeapDumps) {
    // Build first heap dump
    BytesView heapDump1;
    {
        JavaScriptHeapDumpBuilder builder;
        builder.beginNode(JavaScriptHeapDumpNodeType::ARRAY, STRING_LITERAL("Array"), 1, 12);
        builder.appendEdgeToValue(JavaScriptHeapDumpEdgeType::ELEMENT,
                                  JavaScriptHeapEdgeIdentifier::indexed(0),
                                  JavaScriptHeapDumpNodeType::STRING,
                                  2,
                                  "Hello",
                                  40);

        heapDump1 = builder.build();
    }

    // Build second heap dump
    BytesView heapDump2;
    {
        JavaScriptHeapDumpBuilder builder;
        builder.beginNode(JavaScriptHeapDumpNodeType::OBJECT, STRING_LITERAL("Object"), 1, 16);
        builder.appendEdgeToValue(JavaScriptHeapDumpEdgeType::PROPERTY,
                                  JavaScriptHeapEdgeIdentifier::named(STRING_LITERAL("name")),
                                  JavaScriptHeapDumpNodeType::STRING,
                                  2,
                                  "World",
                                  20);

        heapDump2 = builder.build();
    }

    // Merge heap dumps
    SimpleExceptionTracker exceptionTracker;
    JavaScriptHeapDumpBuilder builder;
    builder.appendDump(heapDump1.data(), heapDump1.size(), exceptionTracker);

    ASSERT_NO_EXCEPTION(exceptionTracker);

    builder.appendDump(heapDump2.data(), heapDump2.size(), exceptionTracker);
    ASSERT_NO_EXCEPTION(exceptionTracker);

    auto finalHeapDump = builder.build();

    std::string str = std::string(reinterpret_cast<const char*>(finalHeapDump.data()), finalHeapDump.size());

    auto result = jsonToValue(finalHeapDump.data(), finalHeapDump.size());

    ASSERT_TRUE(result) << result.description();

    std::string_view expectedJSON =
        R"({"snapshot":{"meta":{"node_fields":["type","name","id","self_size","edge_count","trace_node_id","detachedness"],"node_types":[["hidden","array","string","object","code","closure","regexp","number","native","synthetic","symbol","bigint","object shape"],"string","number","number","number","number","number"],"edge_fields":["type","name_or_index","to_node"],"edge_types":[["context","element","property","internal","hidden","shortcut","weak"],"string_or_number","node"]},"node_count":4,"edge_count":2,"trace_function_count":0},
"nodes":[1,0,1,12,1,0,0,
2,1,2,40,0,0,0,
3,2,3,16,1,0,0,
2,4,4,20,0,0,0],
"edges":[1,0,7,
2,3,21],
"strings":[
"Array",
"Hello",
"Object",
"name",
"World"
]}
)";

    auto expectedJSONValue = jsonToValue(expectedJSON);
    ASSERT_TRUE(expectedJSONValue) << expectedJSONValue.description();

    ASSERT_EQ(expectedJSONValue.value(), result.value());
}

} // namespace ValdiTest
