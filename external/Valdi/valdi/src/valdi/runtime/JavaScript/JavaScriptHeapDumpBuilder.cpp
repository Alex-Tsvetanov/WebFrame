//
//  JavaScriptHeapDumpBuilder.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 8/17/23.
//

#include "valdi/runtime/JavaScript/JavaScriptHeapDumpBuilder.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Utils/JSONReader.hpp"
#include "valdi_core/cpp/Utils/JSONWriter.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueTypedObject.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

#include <utility>

namespace Valdi {

JavaScriptHeapEdgeIdentifier::JavaScriptHeapEdgeIdentifier(Value&& value) : _value(std::move(value)) {}

JavaScriptHeapEdgeIdentifier::JavaScriptHeapEdgeIdentifier() = default;

bool JavaScriptHeapEdgeIdentifier::isNamed() const {
    return _value.isString();
}

bool JavaScriptHeapEdgeIdentifier::isIndexed() const {
    return _value.isInt();
}

StringBox JavaScriptHeapEdgeIdentifier::getName() const {
    return _value.toStringBox();
}

int32_t JavaScriptHeapEdgeIdentifier::getIndex() const {
    return _value.toInt();
}

bool JavaScriptHeapEdgeIdentifier::operator==(const JavaScriptHeapEdgeIdentifier& other) const {
    return _value == other._value;
}

bool JavaScriptHeapEdgeIdentifier::operator!=(const JavaScriptHeapEdgeIdentifier& other) const {
    return !(*this == other);
}

JavaScriptHeapEdgeIdentifier JavaScriptHeapEdgeIdentifier::named(const StringBox& name) {
    return JavaScriptHeapEdgeIdentifier(Value(name));
}

JavaScriptHeapEdgeIdentifier JavaScriptHeapEdgeIdentifier::named(const char* name) {
    return named(StringCache::getGlobal().makeString(std::string_view(name)));
}

JavaScriptHeapEdgeIdentifier JavaScriptHeapEdgeIdentifier::indexed(size_t index) {
    return JavaScriptHeapEdgeIdentifier(Value(static_cast<int32_t>(index)));
}

static bool findValueIndex(const ValueArray* array, const char* str, size_t& output) {
    auto key = StringCache::makeStringFromCLiteral(str);

    for (size_t i = 0; i < array->size(); i++) {
        if ((*array)[i].toStringBox() == key) {
            output = i;
            return true;
        }
    }

    return false;
}

template<typename T>
static void populateAssociationIndex(const ValueArray* values,
                                     T defaultValue,
                                     std::initializer_list<std::pair<T, const char*>> enumValues,
                                     SmallVectorBase<T>& output) {
    for (const auto& value : *values) {
        auto valueStr = value.toStringBox();
        auto resolvedEnumValue = defaultValue;
        for (size_t i = 0; i < enumValues.size(); i++) {
            auto enumValueStr = std::string_view(enumValues.begin()[i].second);
            if (valueStr == enumValueStr) {
                resolvedEnumValue = enumValues.begin()[i].first;
                break;
            }
        }

        output.emplace_back(resolvedEnumValue);
    }
}

JavaScriptHeapDumpFieldsIndexes::JavaScriptHeapDumpFieldsIndexes() = default;
JavaScriptHeapDumpFieldsIndexes::~JavaScriptHeapDumpFieldsIndexes() = default;

bool JavaScriptHeapDumpFieldsIndexes::populate(const ValueArray* nodeFields,
                                               const ValueArray* nodeTypes,
                                               const ValueArray* edgeFields,
                                               const ValueArray* edgeTypes) {
    if (!resolveFieldsIndexes(nodeFields, edgeFields)) {
        return false;
    }

    fieldsCountPerNode = nodeFields->size();
    fieldsCountPerEdge = edgeFields->size();

    if (nodeTypes->size() != nodeFields->size() || edgeFields->size() != edgeTypes->size()) {
        return false;
    }

    if (!resolveNodeTypeByIndex(nodeTypes)) {
        return false;
    }

    if (!resolveEdgeTypeByIndex(edgeTypes)) {
        return false;
    }

    _populated = true;

    return true;
}

bool JavaScriptHeapDumpFieldsIndexes::resolveFieldsIndexes(const ValueArray* nodeFields, const ValueArray* edgeFields) {
    if (!findValueIndex(nodeFields, "type", nodes.type)) {
        return false;
    }
    if (!findValueIndex(nodeFields, "name", nodes.name)) {
        return false;
    }

    if (!findValueIndex(nodeFields, "id", nodes.id)) {
        return false;
    }

    if (!findValueIndex(nodeFields, "self_size", nodes.selfSize)) {
        return false;
    }

    if (!findValueIndex(nodeFields, "edge_count", nodes.edgeCount)) {
        return false;
    }

    if (!findValueIndex(edgeFields, "type", edges.type)) {
        return false;
    }

    if (!findValueIndex(edgeFields, "name_or_index", edges.nameOrIndex)) {
        return false;
    }

    return findValueIndex(edgeFields, "to_node", edges.toNode);
}

bool JavaScriptHeapDumpFieldsIndexes::resolveNodeTypeByIndex(const ValueArray* nodeTypes) {
    if (nodes.type >= nodeTypes->size()) {
        return false;
    }

    const auto* nodeTypesDef = (*nodeTypes)[nodes.type].getArray();
    if (nodeTypesDef == nullptr) {
        return false;
    }

    populateAssociationIndex(nodeTypesDef,
                             JavaScriptHeapDumpNodeType::SYNTHETIC,
                             {{JavaScriptHeapDumpNodeType::HIDDEN, "hidden"},
                              {JavaScriptHeapDumpNodeType::ARRAY, "array"},
                              {JavaScriptHeapDumpNodeType::STRING, "string"},
                              {JavaScriptHeapDumpNodeType::OBJECT, "object"},
                              {JavaScriptHeapDumpNodeType::CODE, "code"},
                              {JavaScriptHeapDumpNodeType::CLOSURE, "closure"},
                              {JavaScriptHeapDumpNodeType::REGEXP, "regexp"},
                              {JavaScriptHeapDumpNodeType::NUMBER, "number"},
                              {JavaScriptHeapDumpNodeType::NATIVE, "native"},
                              {JavaScriptHeapDumpNodeType::SYNTHETIC, "synthetic"},
                              {JavaScriptHeapDumpNodeType::SYMBOL, "symbol"},
                              {JavaScriptHeapDumpNodeType::BIGINT, "bigint"},
                              {JavaScriptHeapDumpNodeType::OBJECT_SHAPE, "object shape"}},
                             nodeTypeByIndex);

    return true;
}

bool JavaScriptHeapDumpFieldsIndexes::resolveEdgeTypeByIndex(const ValueArray* edgeTypes) {
    if (edges.type >= edgeTypes->size()) {
        return false;
    }

    const auto* edgeTypesDef = (*edgeTypes)[edges.type].getArray();
    if (edgeTypesDef == nullptr) {
        return false;
    }

    populateAssociationIndex(edgeTypesDef,
                             JavaScriptHeapDumpEdgeType::INTERNAL,
                             {
                                 {JavaScriptHeapDumpEdgeType::CONTEXT, "context"},
                                 {JavaScriptHeapDumpEdgeType::ELEMENT, "element"},
                                 {JavaScriptHeapDumpEdgeType::PROPERTY, "property"},
                                 {JavaScriptHeapDumpEdgeType::INTERNAL, "internal"},
                                 {JavaScriptHeapDumpEdgeType::HIDDEN, "hidden"},
                                 {JavaScriptHeapDumpEdgeType::SHORTCUT, "shortcut"},
                                 {JavaScriptHeapDumpEdgeType::WEAK, "weak"},
                             },
                             edgeTypeByIndex);

    return true;
}

JavaScriptHeapDumpParser::JavaScriptHeapDumpParser(const Byte* data,
                                                   size_t length,
                                                   ExceptionTracker& exceptionTracker) {
    JSONReader reader(std::string_view(reinterpret_cast<const char*>(data), length));

    parseHeapDump(reader);

    if (reader.hasError()) {
        _nodes.clear();
        _edges.clear();
        exceptionTracker.onError(reader.getError());
        return;
    }
}

JavaScriptHeapDumpParser::~JavaScriptHeapDumpParser() = default;

bool JavaScriptHeapDumpParser::parseSnapshot(const Value& snapshot, int32_t& nodeCount, int32_t& edgeCount) {
    nodeCount = snapshot.getMapValue("node_count").toInt();
    edgeCount = snapshot.getMapValue("edge_count").toInt();
    auto meta = snapshot.getMapValue("meta");
    const auto* nodeFields = meta.getMapValue("node_fields").getArray();
    const auto* nodeTypes = meta.getMapValue("node_types").getArray();
    const auto* edgeFields = meta.getMapValue("edge_fields").getArray();
    const auto* edgeTypes = meta.getMapValue("edge_types").getArray();

    if (nodeFields == nullptr || nodeTypes == nullptr || edgeFields == nullptr || edgeTypes == nullptr) {
        return false;
    }

    return _fieldsIndexes.populate(nodeFields, nodeTypes, edgeFields, edgeTypes);
}

void JavaScriptHeapDumpParser::parseHeapDump(JSONReader& reader) {
    if (!reader.parseBeginObject()) {
        return;
    }

    bool isFirst = true;
    std::string key;
    int32_t nodeCount = 0;
    int32_t edgeCount = 0;

    while (!reader.tryParseEndObject()) {
        if (reader.hasError()) {
            return;
        }

        if (isFirst) {
            isFirst = false;
        } else if (!reader.parseComma()) {
            return;
        }

        key.clear();
        if (!reader.parseString(key) || !reader.parseColon()) {
            return;
        }

        if (key == "snapshot") {
            auto snapshotJSON = readValueFromJSONReader(reader);
            if (reader.hasError()) {
                return;
            }
            if (!parseSnapshot(snapshotJSON, nodeCount, edgeCount)) {
                reader.setErrorAtCurrentPosition("Failed to parse snapshot");
                return;
            }
        } else if (key == "nodes") {
            parseNodes(reader, nodeCount);
            if (reader.hasError()) {
                return;
            }
        } else if (key == "edges") {
            parseEdges(reader, edgeCount);
            if (reader.hasError()) {
                return;
            }
        } else if (key == "strings") {
            parseStrings(reader);
            if (reader.hasError()) {
                return;
            }
        } else {
            // Unknown key, just consume the JSONReader
            readValueFromJSONReader(reader);
            if (reader.hasError()) {
                return;
            }
        }
    }

    if (!reader.ensureIsAtEnd()) {
        return;
    }

    validateDump(reader);
}

static bool parseComponents(JSONReader& reader, std::vector<uint32_t>& output, size_t length) {
    output.resize(length);
    auto* outputBegin = output.data();
    auto* outputEnd = outputBegin + length;
    auto* outputIt = outputBegin;

    while (outputIt != outputEnd) {
        if (outputIt != outputBegin && !reader.parseComma()) {
            return false;
        }

        if (!reader.parseUInt(*outputIt)) {
            return false;
        }

        outputIt++;
    }

    return true;
}

void JavaScriptHeapDumpParser::parseNodes(JSONReader& reader, int32_t nodeCount) {
    if (!reader.parseBeginArray()) {
        return;
    }

    auto fieldsCountPerNode = _fieldsIndexes.fieldsCountPerNode;
    std::vector<uint32_t> components;
    if (!parseComponents(reader, components, fieldsCountPerNode * nodeCount)) {
        return;
    }

    auto* componentsPtr = components.data();

    size_t edgesStart = 0;
    auto nodeTypeMaxSize = static_cast<uint32_t>(_fieldsIndexes.nodeTypeByIndex.size());

    for (int32_t i = 0; i < nodeCount; i++) {
        auto typeIndex = componentsPtr[_fieldsIndexes.nodes.type];
        auto nameIndex = componentsPtr[_fieldsIndexes.nodes.name];
        auto id = componentsPtr[_fieldsIndexes.nodes.id];
        auto selfSizeBytes = componentsPtr[_fieldsIndexes.nodes.selfSize];
        auto edgesCount = componentsPtr[_fieldsIndexes.nodes.edgeCount];

        if (typeIndex >= nodeTypeMaxSize) {
            reader.setErrorAtCurrentPosition("Out of bounds edgeType");
            return;
        }

        _nodes.emplace_back(
            this, _fieldsIndexes.nodeTypeByIndex[typeIndex], nameIndex, id, selfSizeBytes, edgesStart, edgesCount);

        edgesStart += edgesCount;
        componentsPtr += fieldsCountPerNode;
    }

    reader.parseEndArray();
}

void JavaScriptHeapDumpParser::parseEdges(JSONReader& reader, int32_t edgeCount) {
    if (!reader.parseBeginArray()) {
        return;
    }

    auto fieldsCountPerEdge = _fieldsIndexes.fieldsCountPerEdge;
    std::vector<uint32_t> components;
    if (!parseComponents(reader, components, fieldsCountPerEdge * edgeCount)) {
        return;
    }
    auto* componentsPtr = components.data();
    auto edgeTypeMaxSize = static_cast<uint32_t>(_fieldsIndexes.edgeTypeByIndex.size());

    for (int32_t i = 0; i < edgeCount; i++) {
        auto edgeTypeIndex = componentsPtr[_fieldsIndexes.edges.type];
        auto edgeNameOrIndex = componentsPtr[_fieldsIndexes.edges.nameOrIndex];
        auto edgeToNode = componentsPtr[_fieldsIndexes.edges.toNode];

        if (edgeTypeIndex >= edgeTypeMaxSize) {
            reader.setErrorAtCurrentPosition("Out of bounds edgeType");
            return;
        }

        _edges.emplace_back(this,
                            _fieldsIndexes.edgeTypeByIndex[edgeTypeIndex],
                            edgeNameOrIndex,
                            edgeToNode / _fieldsIndexes.fieldsCountPerNode);

        componentsPtr += fieldsCountPerEdge;
    }

    reader.parseEndArray();
}

void JavaScriptHeapDumpParser::parseStrings(JSONReader& reader) {
    if (!reader.parseBeginArray()) {
        return;
    }

    std::string str;

    auto first = true;
    while (!reader.tryParseEndArray()) {
        if (first) {
            first = false;
        } else {
            if (!reader.parseComma()) {
                return;
            }
        }

        str.clear();
        if (!reader.parseString(str)) {
            return;
        }

        _strings.emplace_back(StringCache::getGlobal().makeString(str));
    }
}

void JavaScriptHeapDumpParser::validateDump(JSONReader& reader) {
    auto stringsSize = static_cast<uint32_t>(_strings.size());
    auto edgesSize = static_cast<uint32_t>(_edges.size());
    auto nodesSize = static_cast<uint32_t>(_nodes.size());

    for (const auto& node : _nodes) {
        if (node._nameIndex >= stringsSize) {
            reader.setErrorAtCurrentPosition("Out of bounds name");
            return;
        }
        if (node._edgesStart + node._edgesCount > edgesSize) {
            reader.setErrorAtCurrentPosition("Out of bounds edges");
            return;
        }
    }

    for (const auto& edge : _edges) {
        if (edge._type != JavaScriptHeapDumpEdgeType::ELEMENT && edge._nameOrIndex >= stringsSize) {
            reader.setErrorAtCurrentPosition("Out of bounds name");
            return;
        }
        if (edge._nodeIndex >= nodesSize) {
            reader.setErrorAtCurrentPosition("Out of bounds node");
            return;
        }
    }
}

std::span<const JavaScriptHeapDumpParser::Node> JavaScriptHeapDumpParser::getNodes() const {
    return _nodes;
}

JavaScriptHeapDumpParser::Edge::Edge(JavaScriptHeapDumpParser* parser,
                                     JavaScriptHeapDumpEdgeType type,
                                     uint32_t nameOrIndex,
                                     uint32_t nodeIndex)
    : _parser(parser), _type(type), _nameOrIndex(nameOrIndex), _nodeIndex(nodeIndex) {}

JavaScriptHeapDumpEdgeType JavaScriptHeapDumpParser::Edge::getType() const {
    return _type;
}

JavaScriptHeapEdgeIdentifier JavaScriptHeapDumpParser::Edge::getIdentifier() const {
    return _type == JavaScriptHeapDumpEdgeType::ELEMENT ?
               JavaScriptHeapEdgeIdentifier::indexed(_nameOrIndex) :
               JavaScriptHeapEdgeIdentifier::named(_parser->_strings[_nameOrIndex]);
}

const JavaScriptHeapDumpParser::Node& JavaScriptHeapDumpParser::Edge::getNode() const {
    return _parser->_nodes[_nodeIndex];
}

JavaScriptHeapDumpParser::Node::Node(JavaScriptHeapDumpParser* parser,
                                     JavaScriptHeapDumpNodeType type,
                                     uint32_t nameIndex,
                                     uint32_t id,
                                     uint32_t selfSizeBytes,
                                     uint32_t edgesStart,
                                     uint32_t edgesCount)
    : _parser(parser),
      _type(type),
      _nameIndex(nameIndex),
      _id(id),
      _selfSizeBytes(selfSizeBytes),
      _edgesStart(edgesStart),
      _edgesCount(edgesCount) {}

JavaScriptHeapDumpNodeType JavaScriptHeapDumpParser::Node::getType() const {
    return _type;
}

const StringBox& JavaScriptHeapDumpParser::Node::getName() const {
    return _parser->_strings[_nameIndex];
}

uint64_t JavaScriptHeapDumpParser::Node::getId() const {
    return static_cast<uint64_t>(_id);
}

size_t JavaScriptHeapDumpParser::Node::getSelfSizeBytes() const {
    return _selfSizeBytes;
}

std::span<const JavaScriptHeapDumpParser::Edge> JavaScriptHeapDumpParser::Node::getEdges() const {
    return std::span<const JavaScriptHeapDumpParser::Edge>(&_parser->_edges[_edgesStart], _edgesCount);
}

static constexpr size_t kElementsPerNode = 7;

JavaScriptHeapDumpBuilder::JavaScriptHeapDumpBuilder() = default;
JavaScriptHeapDumpBuilder::~JavaScriptHeapDumpBuilder() = default;

static void writeSnapshot(size_t nodeCount, size_t edgeCount, JSONWriter& writer) {
    writer.writeBeginObject();

    {
        writer.writeProperty("meta");
        writer.writeBeginObject();

        {
            writer.writeProperty("node_fields");
            writer.writeStringArray({"type", "name", "id", "self_size", "edge_count", "trace_node_id", "detachedness"});
            writer.writeComma();

            writer.writeProperty("node_types");
            writer.writeBeginArray();
            {
                writer.writeStringArray({"hidden",
                                         "array",
                                         "string",
                                         "object",
                                         "code",
                                         "closure",
                                         "regexp",
                                         "number",
                                         "native",
                                         "synthetic",
                                         "symbol",
                                         "bigint",
                                         "object shape"});
                writer.writeComma();
                writer.writeCommaDelimitedStrings({"string", "number", "number", "number", "number", "number"});
            }
            writer.writeEndArray();
            writer.writeComma();

            writer.writeProperty("edge_fields");
            writer.writeStringArray({"type", "name_or_index", "to_node"});
            writer.writeComma();

            writer.writeProperty("edge_types");
            writer.writeBeginArray();
            {
                writer.writeStringArray({"context", "element", "property", "internal", "hidden", "shortcut", "weak"});
                writer.writeComma();
                writer.writeCommaDelimitedStrings({"string_or_number", "node"});
            }
            writer.writeEndArray();
        }

        writer.writeEndObject();
        writer.writeComma();

        writer.writeProperty("node_count");
        writer.writeInt(static_cast<int64_t>(nodeCount));
        writer.writeComma();

        writer.writeProperty("edge_count");
        writer.writeInt(static_cast<int64_t>(edgeCount));
        writer.writeComma();

        writer.writeProperty("trace_function_count");
        writer.writeInt(static_cast<int32_t>(0));
    }

    writer.writeEndObject();
}

BytesView JavaScriptHeapDumpBuilder::build() {
    SC_ASSERT(_nodes.size() == _nodeById.size());

    auto output = makeShared<ByteBuffer>();
    JSONWriter writer(*output);

    writer.writeBeginObject();

    writer.writeProperty("snapshot");
    writeSnapshot(_nodes.size(), _edges.size(), writer);
    writer.writeComma();
    writer.writeNewLine();

    writer.writeProperty("nodes");
    writer.writeArray(_nodes, [&](const JavaScriptHeapDumpBuilder::Node& node) {
        /**
         0    type    The type of node. See Node types, below.
         1    name    The name of the node. This is a number that's the index in the top-level strings array. To find
         the actual name, use the index number to look up the string in the top-level strings array.
         2    id    The node's unique ID.
         3    self_size    The node's size in bytes.
         4    edge_count    The number of edges connected to this node.
         5    trace_node_id    The ID of the trace node
         6    detachedness    Whether this node can be reached from the window global object. 0 means the node is not
         detached; the node can be reached from the window global object. 1 means the node is detached; the node can't
         be reached from the window global object.
         */
        writer.writeInt(node.type);
        writer.writeComma();
        writer.writeInt(node.name);
        writer.writeComma();
        writer.writeInt(node.id);
        writer.writeComma();
        writer.writeInt(node.selfSize);
        writer.writeComma();
        writer.writeInt(node.edgeCount);
        writer.writeComma();
        writer.writeInt(static_cast<int32_t>(0));
        writer.writeComma();
        writer.writeInt(static_cast<int32_t>(0));
    });
    writer.writeComma();
    writer.writeNewLine();

    writer.writeProperty("edges");
    writer.writeArray(_edges, [&](const JavaScriptHeapDumpBuilder::Edge& edge) {
        /**
         0    type    The type of edge. See Edge types to find out what are the possible types.
         1    name_or_index    This can be a number or a string. If it's a number, it corresponds to the index in the
         top-level strings array, where the name of the edge can be found. 2    to_node    The index within the nodes
         array that this edge is connected to.
         */
        const auto& indexedNode = _nodeById[edge.nodeId - 1];
        // We should have visited the node and resolved its index
        SC_ASSERT(indexedNode.nodeIndex.has_value());

        writer.writeInt(edge.type);
        writer.writeComma();
        writer.writeInt(edge.nameOrIndex);
        writer.writeComma();
        writer.writeInt(static_cast<int32_t>(indexedNode.nodeIndex.value() * kElementsPerNode));
    });
    writer.writeComma();
    writer.writeNewLine();

    writer.writeProperty("strings");
    writer.writeArray(_stringTable, [&](const StringBox& str) { writer.writeString(str.toStringView()); });

    writer.writeEndObject();

    return output->toBytesView();
}

void JavaScriptHeapDumpBuilder::appendDump(const Byte* data, size_t length, ExceptionTracker& exceptionTracker) {
    JavaScriptHeapDumpParser parser(data, length, exceptionTracker);
    if (!exceptionTracker) {
        return;
    }

    // Use the first 16 bits of our id as a sequence for the heap we are about to append
    // This ensures that all the ids we are processing within this dump will remain
    // unique.
    auto nodeIdMask = static_cast<uint64_t>(_appendDumpIndex++) << 48;

    for (const auto& node : parser.getNodes()) {
        beginNode(node.getType(), node.getName(), node.getId() | nodeIdMask, node.getSelfSizeBytes());

        for (const auto& edge : node.getEdges()) {
            appendEdge(edge.getType(), edge.getIdentifier(), edge.getNode().getId() | nodeIdMask);
        }
    }
}

void JavaScriptHeapDumpBuilder::beginNode(JavaScriptHeapDumpNodeType type,
                                          const StringBox& name,
                                          void* nodePtr,
                                          size_t selfSizeBytes) {
    beginNode(type, name, reinterpret_cast<uint64_t>(nodePtr), selfSizeBytes);
}

void JavaScriptHeapDumpBuilder::beginNode(JavaScriptHeapDumpNodeType type,
                                          const StringBox& name,
                                          uint64_t nodeId,
                                          size_t selfSizeBytes) {
    auto& referencedNode = getReferencedNode(nodeId);
    SC_ASSERT(!referencedNode.nodeIndex.has_value());
    auto newNodeIndex = _nodes.size();
    referencedNode.nodeIndex = {newNodeIndex};

    auto& node = _nodes.emplace_back();

    node.type = static_cast<int32_t>(type);
    node.name = getStringTableIndex(name);
    node.id = referencedNode.id;
    node.selfSize = static_cast<int32_t>(selfSizeBytes);

    _currentNode = &node;
    _currentNodeIndex = newNodeIndex;
}

void JavaScriptHeapDumpBuilder::appendEdge(JavaScriptHeapDumpEdgeType type,
                                           const JavaScriptHeapEdgeIdentifier& identifier,
                                           void* nodePtr) {
    appendEdge(type, identifier, reinterpret_cast<uint64_t>(nodePtr));
}

void JavaScriptHeapDumpBuilder::appendEdge(JavaScriptHeapDumpEdgeType type,
                                           const JavaScriptHeapEdgeIdentifier& identifier,
                                           uint64_t nodeId) {
    auto& edge = _edges.emplace_back();
    edge.type = static_cast<int32_t>(type);
    edge.nodeId = getReferencedNode(nodeId).id;
    _currentNode->edgeCount++;

    if (identifier.isNamed()) {
        edge.nameOrIndex = getStringTableIndex(identifier.getName());
    } else {
        edge.nameOrIndex = identifier.getIndex();
    }
}

void JavaScriptHeapDumpBuilder::appendEdgeToValue(JavaScriptHeapDumpEdgeType type,
                                                  const JavaScriptHeapEdgeIdentifier& identifier,
                                                  JavaScriptHeapDumpNodeType nodeType,
                                                  void* nodePtr,
                                                  const char* stringValue,
                                                  size_t valueSelfSizeBytes) {
    appendEdgeToValue(type, identifier, nodeType, reinterpret_cast<uint64_t>(nodePtr), stringValue, valueSelfSizeBytes);
}

void JavaScriptHeapDumpBuilder::appendEdgeToValue(JavaScriptHeapDumpEdgeType type,
                                                  const JavaScriptHeapEdgeIdentifier& identifier,
                                                  JavaScriptHeapDumpNodeType nodeType,
                                                  uint64_t nodeId,
                                                  const char* stringValue,
                                                  size_t valueSelfSizeBytes) {
    JavaScriptHeapDumpBuilder::ReferencedNode* referencedNode = nullptr;

    if (nodeId != 0) {
        referencedNode = &getReferencedNode(nodeId);
    } else {
        referencedNode = &appendReferencedNode();
    }

    if (!referencedNode->nodeIndex) {
        referencedNode->nodeIndex = {_nodes.size()};

        auto internedName = StringCache::getGlobal().makeString(std::string_view(stringValue));

        auto& node = _nodes.emplace_back();

        node.type = static_cast<int32_t>(nodeType);
        node.name = getStringTableIndex(internedName);
        node.id = referencedNode->id;
        node.selfSize = static_cast<int32_t>(valueSelfSizeBytes);

        // Restore current node
        _currentNode = &_nodes[_currentNodeIndex];
    }

    auto& edge = _edges.emplace_back();
    edge.type = static_cast<int32_t>(type);
    edge.nodeId = referencedNode->id;

    if (identifier.isNamed()) {
        edge.nameOrIndex = getStringTableIndex(identifier.getName());
    } else {
        edge.nameOrIndex = identifier.getIndex();
    }

    _currentNode->edgeCount++;
}

JavaScriptHeapDumpBuilder::ReferencedNode& JavaScriptHeapDumpBuilder::getReferencedNode(uint64_t nodeId) {
    auto it = _nodePtrToIndex.find(nodeId);
    if (it != _nodePtrToIndex.end()) {
        return _nodeById[it->second];
    }

    auto& referencedNode = appendReferencedNode();
    _nodePtrToIndex[nodeId] = static_cast<size_t>(referencedNode.id - 1);

    return referencedNode;
}

JavaScriptHeapDumpBuilder::ReferencedNode& JavaScriptHeapDumpBuilder::appendReferencedNode() {
    auto nodeIndex = _nodeById.size();
    auto& referencedNode = _nodeById.emplace_back();
    referencedNode.id = static_cast<int32_t>(nodeIndex + 1);

    return referencedNode;
}

int32_t JavaScriptHeapDumpBuilder::getStringTableIndex(const StringBox& str) {
    const auto& it = _stringToStringTableIndex.find(str);
    if (it != _stringToStringTableIndex.end()) {
        return it->second;
    }

    auto index = static_cast<int32_t>(_stringTable.size());
    _stringTable.emplace_back(str);
    _stringToStringTableIndex[str] = index;

    return index;
}

} // namespace Valdi
