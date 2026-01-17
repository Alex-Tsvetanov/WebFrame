//
//  YogaAttributes.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/13/18.
//

#include "valdi/runtime/Attributes/Yoga/YogaAttributes.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/CompositeAttributeUtils.hpp"

#include "valdi/runtime/Attributes/Yoga/YGEdgesAttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/Yoga/YGEnumAttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/Yoga/YGFloatOptionalAttributeHandlerDelegate.hpp"
#include "valdi/runtime/Attributes/Yoga/YGValueAttributeHandlerDelegate.hpp"

#include "valdi/runtime/Context/ViewNode.hpp"

#include <array>

namespace Valdi {

static void populateAssociativeEnumMap(FlatMap<StringBox, int>& associativeMap,
                                       int enumCount,
                                       const char* (*toStringFunction)(int)) {
    associativeMap.reserve(static_cast<size_t>(enumCount));

    for (int i = 0; i < enumCount; i++) {
        auto key = STRING_LITERAL(toStringFunction(i));
        associativeMap[key] = i;
    }
}

template<typename Type, const char* (*toStringFunction)(Type)>
static void populateAssociativeEnumMap(FlatMap<StringBox, int>& associateMap) {
    populateAssociativeEnumMap(associateMap, facebook::yoga::enums::count<Type>(), [](int index) {
        return toStringFunction(static_cast<Type>(index));
    });
}

YogaAttributes::YogaAttributes(YGConfig* const yogaConfig, AttributeIds& attributeIds, float pointScale)
    : _defaultYogaNode(Yoga::createNode(yogaConfig)), _attributeIds(attributeIds), _pointScale(pointScale) {
    populateAssociativeEnumMap<YGDirection, YGDirectionToString>(_directionToEnum);
    populateAssociativeEnumMap<YGFlexDirection, YGFlexDirectionToString>(_flexDirectionToEnum);
    populateAssociativeEnumMap<YGJustify, YGJustifyToString>(_justifyToEnum);
    populateAssociativeEnumMap<YGAlign, YGAlignToString>(_alignToEnum);
    populateAssociativeEnumMap<YGPositionType, YGPositionTypeToString>(_positionTypeToEnum);
    populateAssociativeEnumMap<YGWrap, YGWrapToString>(_wrapToEnum);
    populateAssociativeEnumMap<YGOverflow, YGOverflowToString>(_overflowToEnum);
    populateAssociativeEnumMap<YGDisplay, YGDisplayToString>(_displayToEnum);
}

YogaAttributes::~YogaAttributes() {
    YGNodeFree(_defaultYogaNode);
}

void YogaAttributes::bindAttribute(const char* name,
                                   bool isForChildrenNode,
                                   AttributeHandlerById& attributes,
                                   const Ref<YogaAttributeHandlerDelegate>& delegate) {
    configureDelegate(isForChildrenNode, delegate);
    auto attributeName = StringCache::getGlobal().makeStringFromLiteral(name);
    auto attributeId = _attributeIds.getIdForName(attributeName);
    attributes[attributeId] = AttributeHandler(attributeId, attributeName, delegate, nullptr, false, false);
}

template<YGEdge edge>
static YGNodeValueGetterSetter<YGCompactValue> makePositionEdgeGetterSetter() {
    return YGNodeValueGetterSetter<YGCompactValue>(
        [](YGNodeRef node) -> YGCompactValue { return node->getStyle().position()[edge]; },
        [](YGNodeRef node, YGCompactValue value) { node->getStyle().position()[edge] = value; });
}

void YogaAttributes::bindPositionAttributes(AttributeHandlerById& attributes) {
    bindYGValueAttribute("top", false, attributes, makePositionEdgeGetterSetter<YGEdgeTop>());
    bindYGValueAttribute("right", false, attributes, makePositionEdgeGetterSetter<YGEdgeEnd>());
    bindYGValueAttribute("bottom", false, attributes, makePositionEdgeGetterSetter<YGEdgeBottom>());
    bindYGValueAttribute("left", false, attributes, makePositionEdgeGetterSetter<YGEdgeStart>());
}

static std::array<std::pair<YGEdge, std::string>, 4> getAllEdges() {
    return {
        std::make_pair(YGEdgeTop, "top"),
        std::make_pair(YGEdgeEnd, "right"),
        std::make_pair(YGEdgeBottom, "bottom"),
        std::make_pair(YGEdgeStart, "left"),
    };
}

void YogaAttributes::bindEnumAttribute(const char* name,
                                       bool isForChildrenNode,
                                       AttributeHandlerById& attributes,
                                       const FlatMap<StringBox, int>& associateMap,
                                       YGNodeValueGetterSetter<int> getterSetter) {
    bindAttribute(
        name, isForChildrenNode, attributes, makeShared<YGEnumAttributeHandlerDelegate>(associateMap, getterSetter));
}

void YogaAttributes::bindYGOptionalAttribute(const char* name,
                                             bool isForChildrenNode,
                                             AttributeHandlerById& attributes,
                                             YGNodeValueGetterSetter<YGFloatOptional> getterSetter) {
    bindAttribute(
        name, isForChildrenNode, attributes, makeShared<YGFloatOptionalAttributeHandlerDelegate>(getterSetter));
}

void YogaAttributes::bindYGValueAttribute(const char* name,
                                          bool isForChildrenNode,
                                          AttributeHandlerById& attributes,
                                          YGNodeValueGetterSetter<YGCompactValue> getterSetter) {
    bindAttribute(name, isForChildrenNode, attributes, makeShared<YGValueAttributeHandlerDelegate>(getterSetter));
}

template<typename GetEdges>
void YogaAttributes::bindPaddingOrMarginAttributes(std::string_view name,
                                                   bool isForChildrenNode,
                                                   AttributeHandlerById& attributes,
                                                   GetEdges&& getEdges) {
    bindEdgeAttributes(name,
                       isForChildrenNode,
                       attributes,
                       makeShared<YGEdgesAttributeHandlerDelegate<GetEdges>>(std::forward<GetEdges>(getEdges)));
}

void YogaAttributes::bindEdgeAttributes(std::string_view name,
                                        bool isForChildrenNode,
                                        AttributeHandlerById& attributes,
                                        const Ref<YogaAttributeHandlerDelegate>& delegate) {
    std::vector<CompositeAttributePart> parts;

    parts.emplace_back(_attributeIds.getIdForName(name), true);

    for (const auto& edge : getAllEdges()) {
        auto correctedEdge = edge.second;
        correctedEdge[0] = toupper(correctedEdge[0]);
        auto attributeName = std::string(name) + correctedEdge;

        auto attributeId = _attributeIds.getIdForName(StringCache::getGlobal().makeString(std::move(attributeName)));
        parts.emplace_back(attributeId, true);
    }

    auto compositeName = StringCache::getGlobal().makeString(std::string("_") + std::string(name));
    auto compositeId = _attributeIds.getIdForName(compositeName);

    auto compositeAttribute = Valdi::makeShared<CompositeAttribute>(compositeId, compositeName, std::move(parts));

    configureDelegate(isForChildrenNode, delegate);
    Valdi::bindCompositeAttribute(_attributeIds, attributes, compositeAttribute, delegate, false);
}

void YogaAttributes::configureDelegate(bool isForChildrenNode, const Ref<YogaAttributeHandlerDelegate>& delegate) {
    delegate->_isForChildrenNode = isForChildrenNode;
    delegate->_yogaAttributes = strongSmallRef(this);
}

void YogaAttributes::bind(AttributeHandlerById& attributes) {
    bindEnumAttribute(
        "direction",
        true,
        attributes,
        _directionToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().direction()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().direction() = static_cast<YGDirection>(enumValue); }));

    bindEnumAttribute(
        "flexDirection",
        true,
        attributes,
        _flexDirectionToEnum,
        YGNodeValueGetterSetter<int>([](YGNodeRef node) { return static_cast<int>(node->getStyle().flexDirection()); },
                                     [](YGNodeRef node, int enumValue) {
                                         node->getStyle().flexDirection() = static_cast<YGFlexDirection>(enumValue);
                                     }));

    bindEnumAttribute(
        "justifyContent",
        true,
        attributes,
        _justifyToEnum,
        YGNodeValueGetterSetter<int>([](YGNodeRef node) { return static_cast<int>(node->getStyle().justifyContent()); },
                                     [](YGNodeRef node, int enumValue) {
                                         node->getStyle().justifyContent() = static_cast<YGJustify>(enumValue);
                                     }));

    bindEnumAttribute(
        "alignContent",
        true,
        attributes,
        _alignToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().alignContent()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().alignContent() = static_cast<YGAlign>(enumValue); }));

    bindEnumAttribute(
        "alignItems",
        true,
        attributes,
        _alignToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().alignItems()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().alignItems() = static_cast<YGAlign>(enumValue); }));

    bindEnumAttribute(
        "alignSelf",
        false,
        attributes,
        _alignToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().alignSelf()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().alignSelf() = static_cast<YGAlign>(enumValue); }));

    bindEnumAttribute(
        "position",
        false,
        attributes,
        _positionTypeToEnum,
        YGNodeValueGetterSetter<int>([](YGNodeRef node) { return static_cast<int>(node->getStyle().positionType()); },
                                     [](YGNodeRef node, int enumValue) {
                                         node->getStyle().positionType() = static_cast<YGPositionType>(enumValue);
                                     }));

    bindEnumAttribute(
        "flexWrap",
        true,
        attributes,
        _wrapToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().flexWrap()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().flexWrap() = static_cast<YGWrap>(enumValue); }));

    bindEnumAttribute(
        "overflow",
        true,
        attributes,
        _overflowToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().overflow()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().overflow() = static_cast<YGOverflow>(enumValue); }));

    bindEnumAttribute(
        "display",
        false,
        attributes,
        _displayToEnum,
        YGNodeValueGetterSetter<int>(
            [](YGNodeRef node) { return static_cast<int>(node->getStyle().display()); },
            [](YGNodeRef node, int enumValue) { node->getStyle().display() = static_cast<YGDisplay>(enumValue); }));

    bindYGOptionalAttribute("flexGrow",
                            false,
                            attributes,
                            YGNodeValueGetterSetter<YGFloatOptional>(
                                [](YGNodeRef node) -> YGFloatOptional { return node->getStyle().flexGrow(); },
                                [](YGNodeRef node, YGFloatOptional value) { node->getStyle().flexGrow() = value; }));

    bindYGOptionalAttribute("flexShrink",
                            false,
                            attributes,
                            YGNodeValueGetterSetter<YGFloatOptional>(
                                [](YGNodeRef node) -> YGFloatOptional { return node->getStyle().flexShrink(); },
                                [](YGNodeRef node, YGFloatOptional value) { node->getStyle().flexShrink() = value; }));

    bindYGValueAttribute("flexBasis",
                         false,
                         attributes,
                         YGNodeValueGetterSetter<YGCompactValue>(
                             [](YGNodeRef node) -> YGCompactValue { return node->getStyle().flexBasis(); },
                             [](YGNodeRef node, YGCompactValue value) { node->getStyle().flexBasis() = value; }));

    bindPositionAttributes(attributes);

    bindPaddingOrMarginAttributes(
        "margin", false, attributes, [](YGNodeRef node) -> auto { return node->getStyle().margin(); });
    bindPaddingOrMarginAttributes(
        "padding", true, attributes, [](YGNodeRef node) -> auto { return node->getStyle().padding(); });

    bindYGValueAttribute(
        "borderTopWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().border()[YGEdgeTop]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().border()[YGEdgeTop] = value; }));

    bindYGValueAttribute(
        "borderRightWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().border()[YGEdgeEnd]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().border()[YGEdgeEnd] = value; }));

    bindYGValueAttribute(
        "borderBottomWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().border()[YGEdgeBottom]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().border()[YGEdgeBottom] = value; }));

    bindYGValueAttribute(
        "borderLeftWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().border()[YGEdgeStart]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().border()[YGEdgeStart] = value; }));

    bindYGValueAttribute(
        "borderWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().border()[YGEdgeAll]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().border()[YGEdgeAll] = value; }));

    bindYGValueAttribute(
        "width",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().dimensions()[YGDimensionWidth]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().dimensions()[YGDimensionWidth] = value; }));

    bindYGValueAttribute(
        "height",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().dimensions()[YGDimensionHeight]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().dimensions()[YGDimensionHeight] = value; }));

    bindYGValueAttribute(
        "minWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().minDimensions()[YGDimensionWidth]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().minDimensions()[YGDimensionWidth] = value; }));

    bindYGValueAttribute(
        "minHeight",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().minDimensions()[YGDimensionHeight]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().minDimensions()[YGDimensionHeight] = value; }));

    bindYGValueAttribute(
        "maxWidth",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().maxDimensions()[YGDimensionWidth]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().maxDimensions()[YGDimensionWidth] = value; }));

    bindYGValueAttribute(
        "maxHeight",
        false,
        attributes,
        YGNodeValueGetterSetter<YGCompactValue>(
            [](YGNodeRef node) -> YGCompactValue { return node->getStyle().maxDimensions()[YGDimensionHeight]; },
            [](YGNodeRef node, YGCompactValue value) { node->getStyle().maxDimensions()[YGDimensionHeight] = value; }));

    bindYGOptionalAttribute("aspectRatio",
                            false,
                            attributes,
                            YGNodeValueGetterSetter<YGFloatOptional>(
                                [](YGNodeRef node) -> YGFloatOptional { return node->getStyle().aspectRatio(); },
                                [](YGNodeRef node, YGFloatOptional value) {
                                    if (!value.isUndefined() && value.unwrap() == 0) {
                                        // safety for aspectRatio 0, we make it undefined instead.
                                        node->getStyle().aspectRatio() = YGFloatOptional();
                                    } else {
                                        node->getStyle().aspectRatio() = value;
                                    }
                                }));
}

} // namespace Valdi
