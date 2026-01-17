//
//  JavaScriptHeapDumpBuilder.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 8/17/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/ExceptionTracker.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace Valdi {

class JSONReader;

/**
 The first number in the group of numbers for a node in the nodes array corresponds to its type.
 This number is an index that can be used to lookup the type name in the snapshot.meta.node_types[0] array.
 */
enum class JavaScriptHeapDumpNodeType {
    /**
     A V8 internal element that doesn't correspond directly to a user-controllable JavaScript object.
     In DevTools, these all show up under the category name (system). Even though these objects are internal, they can
     be an important part of retainer paths.
     */
    HIDDEN = 0,
    /**
     Various internal lists, which are displayed in DevTools with the category name (array).
     Like Hidden, this category groups together a variety of things. Many of the objects here are named (object
     properties) or (object elements), indicating that they contain the string-keyed or numeric-keyed properties of a
     JavaScript object.
     */
    ARRAY,
    STRING,
    /**
     Any user-defined object, such as { x: 2 } or new Foo(4). Contexts, which show up in DevTools as system / Context,
     hold variables that had to be allocated on the heap because they're used by a nested function.
     */
    OBJECT,
    /**
     Things that grow proportionally to the amount of script, and/or the number of times that functions run.
     */
    CODE,
    CLOSURE,
    REGEXP,
    NUMBER,
    /**
     Things that are allocated by the Blink rendering engine, rather than by V8. These are mostly DOM items such as
     HTMLDivElement or CSSStyleRule.
     */
    NATIVE,
    /**
     Synthetic nodes don't correspond to anything actually allocated in memory.
     These are used to distinguish the different kinds of garbage-collection (GC) roots.
     */
    SYNTHETIC,
    SYMBOL,
    BIGINT,
    OBJECT_SHAPE,

    COUNT,
};

/**
 The first number in the group of numbers for an edge in the edges array corresponds to its type.
 This number is an index that can be used to lookup the type name in the snapshot.meta.edge_types[0] array.
 */
enum class JavaScriptHeapDumpEdgeType {
    CONTEXT = 0,
    /**
     Object properties where the key is a number.
     */
    ELEMENT,
    PROPERTY,
    INTERNAL,
    /**
     Edges that don't correspond to JavaScript-visible names, but are still important. As an example,
     Function instances have a "context" representing the state of variables that were in scope where the function was
     defined. There is no way for JavaScript code to directly read the "context" of a function, but these edges are
     needed when investigating retainers.
     */
    HIDDEN,
    /**
     An easier-to-read representation of some other path. This type is rarely used.
     For example, if you use Function.prototype.bind to create a bound function with some bound arguments,
     V8 creates a JSBoundFunction, which points to a FixedArray (an internal type), which points to each bound argument.
     When producing a snapshot, V8 adds a shortcut edge from the bound function directly to each bound argument,
     bypassing the FixedArray.
     */
    SHORTCUT,
    /**
     Weak edges don't keep the node they are connected to alive, and thus are omitted from the Retainers view.
     Any object with only weak edges pointing to it can be discarded by the garbage collection (GC).
     */
    WEAK,

    COUNT
};

class JavaScriptHeapEdgeIdentifier {
public:
    JavaScriptHeapEdgeIdentifier();

    bool isNamed() const;
    bool isIndexed() const;

    StringBox getName() const;
    int32_t getIndex() const;

    bool operator==(const JavaScriptHeapEdgeIdentifier& other) const;
    bool operator!=(const JavaScriptHeapEdgeIdentifier& other) const;

    static JavaScriptHeapEdgeIdentifier named(const char* name);
    static JavaScriptHeapEdgeIdentifier named(const StringBox& name);
    static JavaScriptHeapEdgeIdentifier indexed(size_t index);

private:
    Value _value;

    JavaScriptHeapEdgeIdentifier(Value&& value);
};

class JavaScriptHeapDumpBuilder {
public:
    JavaScriptHeapDumpBuilder();
    ~JavaScriptHeapDumpBuilder();

    /**
     Append a new node to the heap dump and mark it as current.
     */
    void beginNode(JavaScriptHeapDumpNodeType type, const StringBox& name, void* nodePtr, size_t selfSizeBytes);

    /**
     Append a new node to the heap dump and mark it as current.
     */
    void beginNode(JavaScriptHeapDumpNodeType type, const StringBox& name, uint64_t nodeId, size_t selfSizeBytes);

    /**
     Append an edge from the current node to the given node pointer. The given node pointer must eventually be
     visited through the beginNode() method before build() is called.
     */
    void appendEdge(JavaScriptHeapDumpEdgeType type, const JavaScriptHeapEdgeIdentifier& identifier, void* nodePtr);

    /**
     Append an edge from the current node to the given node pointer. The given node pointer must eventually be
     visited through the beginNode() method before build() is called.
     */
    void appendEdge(JavaScriptHeapDumpEdgeType type, const JavaScriptHeapEdgeIdentifier& identifier, uint64_t nodeId);

    /**
     Append an edge from the current node to the given value. This will create a node that holds the given value,
     and append an edge from the current node to the newly created node.
     */
    void appendEdgeToValue(JavaScriptHeapDumpEdgeType type,
                           const JavaScriptHeapEdgeIdentifier& identifier,
                           JavaScriptHeapDumpNodeType nodeType,
                           uint64_t nodeId,
                           const char* stringValue,
                           size_t valueSelfSizeBytes);

    /**
     Append an edge from the current node to the given value. This will create a node that holds the given value,
     and append an edge from the current node to the newly created node.
     */
    void appendEdgeToValue(JavaScriptHeapDumpEdgeType type,
                           const JavaScriptHeapEdgeIdentifier& identifier,
                           JavaScriptHeapDumpNodeType nodeType,
                           void* nodePtr,
                           const char* stringValue,
                           size_t valueSelfSizeBytes);

    /**
     Append the entire content of a heap dump.
     */
    void appendDump(const Byte* data, size_t length, ExceptionTracker& exceptionTracker);

    BytesView build();

private:
    /**
     Represents a Node that has been visited through the beginNode() method.
     */
    struct Node {
        /**
         The type of the node.
         */
        int32_t type;
        /**
         The name of the node. This is a number that's the index in the top-level strings array. To find the actual
         name, use the index number to look up the string in the top-level strings array.
         */
        int32_t name;
        /**
         The node's unique ID.
         */
        int32_t id;
        /**
         The node's size in bytes.
         */
        int32_t selfSize;
        /**
         The number of edges connected to this node.
         */
        int32_t edgeCount = 0;
    };

    /**
     Represents an edge between a node to another node.
     */
    struct Edge {
        /**
         The type of edge. See Edge types to find out what are the possible types.
         */
        int32_t type;
        /**
         This can be a number or a string. If it's a number, it corresponds to the index in the top-level strings array,
         where the name of the edge can be found.
         */
        int32_t nameOrIndex;
        int32_t nodeId;
    };

    /**
     Represents a node that has been seen at any point. Every ReferencedNode must eventually be
     visited through the beginNode() method, which is where the nodeIndex will be resolved.
     */
    struct ReferencedNode {
        int32_t id;
        // The index inside the _nodes array.
        // Will be populated as we go through the nodes
        std::optional<size_t> nodeIndex;
    };

    std::vector<Node> _nodes;
    std::vector<Edge> _edges;
    std::vector<ReferencedNode> _nodeById;
    FlatMap<uint64_t, size_t> _nodePtrToIndex;
    std::vector<StringBox> _stringTable;
    FlatMap<StringBox, int32_t> _stringToStringTableIndex;
    size_t _currentNodeIndex = 0;
    size_t _appendDumpIndex = 0;

    Node* _currentNode = nullptr;

    ReferencedNode& getReferencedNode(uint64_t nodeId);
    ReferencedNode& appendReferencedNode();

    int32_t getStringTableIndex(const StringBox& str);
};

/**
 Helper that maps between the field names provided from an arbitrary heap dump
 to the known fields that we know how to process.
 */
struct JavaScriptHeapDumpFieldsIndexes {
    struct NodeFieldsIndexes {
        size_t type;
        size_t name;
        size_t id;
        size_t selfSize;
        size_t edgeCount;
    };

    struct EdgeFieldsIndexes {
        size_t type;
        size_t nameOrIndex;
        size_t toNode;
    };

    NodeFieldsIndexes nodes;
    EdgeFieldsIndexes edges;
    size_t fieldsCountPerNode = 0;
    size_t fieldsCountPerEdge = 0;

    SmallVector<JavaScriptHeapDumpNodeType, 20> nodeTypeByIndex;
    SmallVector<JavaScriptHeapDumpEdgeType, 10> edgeTypeByIndex;

    JavaScriptHeapDumpFieldsIndexes();
    ~JavaScriptHeapDumpFieldsIndexes();

    bool populate(const ValueArray* nodeFields,
                  const ValueArray* nodeTypes,
                  const ValueArray* edgeFields,
                  const ValueArray* edgeTypes);
    bool isPopulated() const;

private:
    bool _populated = false;
    bool resolveFieldsIndexes(const ValueArray* nodeFields, const ValueArray* edgeFields);

    bool resolveNodeTypeByIndex(const ValueArray* nodeTypes);

    bool resolveEdgeTypeByIndex(const ValueArray* edgeTypes);
};

/**
 Helper that can parse a Chrome heap dump and provide all the nodes and edges in a structured format.
 Takes care of validation during parsing.
 */
class JavaScriptHeapDumpParser {
public:
    JavaScriptHeapDumpParser(const Byte* data, size_t length, ExceptionTracker& exceptionTracker);
    ~JavaScriptHeapDumpParser();

    struct Node;
    struct Edge {
    public:
        Edge(JavaScriptHeapDumpParser* parser,
             JavaScriptHeapDumpEdgeType type,
             uint32_t nameOrIndex,
             uint32_t nodeIndex);

        JavaScriptHeapDumpEdgeType getType() const;
        JavaScriptHeapEdgeIdentifier getIdentifier() const;
        const Node& getNode() const;

    private:
        JavaScriptHeapDumpParser* _parser;
        JavaScriptHeapDumpEdgeType _type;
        uint32_t _nameOrIndex;
        uint32_t _nodeIndex;

        friend JavaScriptHeapDumpParser;
    };

    struct Node {
    public:
        Node(JavaScriptHeapDumpParser* parser,
             JavaScriptHeapDumpNodeType type,
             uint32_t nameIndex,
             uint32_t id,
             uint32_t selfSizeBytes,
             uint32_t edgesStart,
             uint32_t edgesCount);

        JavaScriptHeapDumpNodeType getType() const;
        const StringBox& getName() const;
        uint64_t getId() const;
        size_t getSelfSizeBytes() const;

        std::span<const Edge> getEdges() const;

    private:
        JavaScriptHeapDumpParser* _parser;
        JavaScriptHeapDumpNodeType _type;
        uint32_t _nameIndex;
        uint32_t _id;
        uint32_t _selfSizeBytes;
        uint32_t _edgesStart;
        uint32_t _edgesCount;

        friend JavaScriptHeapDumpParser;
    };

    std::span<const Node> getNodes() const;

private:
    JavaScriptHeapDumpFieldsIndexes _fieldsIndexes;
    std::vector<Node> _nodes;
    std::vector<Edge> _edges;
    std::vector<StringBox> _strings;

    friend JavaScriptHeapDumpParser::Node;
    friend JavaScriptHeapDumpParser::Edge;

    void parseHeapDump(JSONReader& reader);
    bool parseSnapshot(const Value& snapshot, int32_t& nodeCount, int32_t& edgeCount);
    void parseNodes(JSONReader& reader, int32_t nodeCount);
    void parseEdges(JSONReader& reader, int32_t edgeCount);
    void parseStrings(JSONReader& reader);
    void validateDump(JSONReader& reader);
};

} // namespace Valdi
