//
//  CSSDocument.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 6/26/19.
//

#include "valdi/runtime/CSS/CSSDocument.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

namespace Valdi {

CSSDocument::CSSDocument(const ResourceId& resourceId, const Valdi::StyleNode& styleNode, AttributeIds& attributeIds)
    : _resourceId(resourceId) {
    populateStyleNode(attributeIds, styleNode, _rootNode);
}

CSSDocument::~CSSDocument() = default;

const MonitoredCssAttributesPtr& CSSDocument::getMonitoredAttributes() const {
    return _monitoredCssAttributes;
}

Value valueFromNodeAttribute(const Valdi::NodeAttribute& attribute) {
    switch (attribute.type()) {
        case Valdi::NodeAttribute_Type_NODE_ATTRIBUTE_TYPE_DOUBLE:
            return Value(attribute.double_value());
        case Valdi::NodeAttribute_Type_NODE_ATTRIBUTE_TYPE_INT:
            return Value(static_cast<int32_t>(attribute.int_value()));
        case Valdi::NodeAttribute_Type_NODE_ATTRIBUTE_TYPE_STRING:
            return Value(StringCache::getGlobal().makeString(attribute.str_value()));
        default:
            return Value();
    }
}

CSSAttribute toCSSAttribute(AttributeIds& attributeIds, const Valdi::NodeAttribute& nodeAttribute) {
    return {.id = attributeIds.getIdForName(Valdi::StringCache::getGlobal().makeString(nodeAttribute.name())),
            .value = valueFromNodeAttribute(nodeAttribute)};
}

void CSSDocument::populateStyleDeclaration(AttributeIds& attributeIds,
                                           const Valdi::StyleDeclaration& styleDeclaration,
                                           CSSStyleDeclaration& currentStyleDeclaration) {
    currentStyleDeclaration.attribute = toCSSAttribute(attributeIds, styleDeclaration.attribute());
    currentStyleDeclaration.id = static_cast<int>(styleDeclaration.id());
    currentStyleDeclaration.priority = static_cast<int>(styleDeclaration.priority());
    currentStyleDeclaration.order = static_cast<int>(styleDeclaration.id());
}

void CSSDocument::populateStyleNode(AttributeIds& attributeIds,
                                    const Valdi::StyleNode& styleNode,
                                    CSSStyleNode& currentNode) {
    for (const auto& decl : styleNode.styles()) {
        auto& newStyle = currentNode.styles.emplace_back();
        populateStyleDeclaration(attributeIds, decl, newStyle);
    }

    if (styleNode.has_ruleindex()) {
        currentNode.ruleIndex = std::make_unique<CSSProcessedRuleIndex>();
        populateRuleIndex(attributeIds, styleNode.ruleindex(), *currentNode.ruleIndex);
    }
}

void CSSDocument::populateMapRule(AttributeIds& attributeIds,
                                  const google::protobuf::RepeatedPtrField<::Valdi::NamedStyleNode>& mapRule,
                                  FlatMap<StringBox, CSSStyleNode>& currentMap) {
    for (const auto& it : mapRule) {
        auto key = StringCache::getGlobal().makeString(it.name());
        auto outIt = currentMap.try_emplace(key);
        populateStyleNode(attributeIds, it.node(), outIt.first->second);
    }
}

void CSSDocument::populateRuleIndex(AttributeIds& attributeIds,
                                    const Valdi::CSSRuleIndex& ruleIndex,
                                    CSSProcessedRuleIndex& currentRuleIndex) {
    populateMapRule(attributeIds, ruleIndex.id_rules(), currentRuleIndex.idRules);
    populateMapRule(attributeIds, ruleIndex.class_rules(), currentRuleIndex.classRules);
    populateMapRule(attributeIds, ruleIndex.tag_rules(), currentRuleIndex.tagRules);

    for (const auto& attributeRule : ruleIndex.attribute_rules()) {
        auto& newAttributeRule = currentRuleIndex.attributeRules.emplace_back();
        newAttributeRule.attribute = toCSSAttribute(attributeIds, attributeRule.attribute());
        newAttributeRule.type = attributeRule.type();

        // We insert into the monitoredCssAttributes set all attributes that we know can
        // impact the resolved rules. To do this we traverse the CSS rule index tree to
        // find all the attributes rules.
        if (_monitoredCssAttributes == nullptr) {
            _monitoredCssAttributes = Valdi::makeShared<FlatSet<AttributeId>>();
        }
        _monitoredCssAttributes->emplace(newAttributeRule.attribute.id);

        populateStyleNode(attributeIds, attributeRule.node(), newAttributeRule.styleNode);
    }

    if (ruleIndex.has_first_child_rule()) {
        currentRuleIndex.firstChildRule = std::make_unique<CSSStyleNode>();
        populateStyleNode(attributeIds, ruleIndex.first_child_rule(), *currentRuleIndex.firstChildRule);
    }

    if (ruleIndex.has_last_child_rule()) {
        currentRuleIndex.lastChildRule = std::make_unique<CSSStyleNode>();
        populateStyleNode(attributeIds, ruleIndex.last_child_rule(), *currentRuleIndex.lastChildRule);
    }

    for (const auto& nthChildRule : ruleIndex.nth_child_rules()) {
        auto& newRule = currentRuleIndex.nthChildRules.emplace_back();
        newRule.n = static_cast<int>(nthChildRule.n());
        newRule.offset = static_cast<int>(nthChildRule.offset());
        populateStyleNode(attributeIds, nthChildRule.node(), newRule.node);
    }

    if (ruleIndex.has_direct_parent_rules()) {
        currentRuleIndex.directParentRules = std::make_unique<CSSProcessedRuleIndex>();
        populateRuleIndex(attributeIds, ruleIndex.direct_parent_rules(), *currentRuleIndex.directParentRules);
    }

    if (ruleIndex.has_ancestor_rules()) {
        currentRuleIndex.ancestorRules = std::make_unique<CSSProcessedRuleIndex>();
        populateRuleIndex(attributeIds, ruleIndex.ancestor_rules(), *currentRuleIndex.ancestorRules);
    }
}

const CSSStyleNode& CSSDocument::getRootNode() const {
    return _rootNode;
}

Result<Ref<CSSAttributes>> CSSDocument::getAttributesForClass(const StringBox& className) const {
    const auto& it = _rootNode.ruleIndex->classRules.find(className);
    if (it == _rootNode.ruleIndex->classRules.end()) {
        return Error(STRING_FORMAT("Could not find CSS rule '{}'", className));
    }

    return Valdi::makeShared<CSSAttributes>(it->second.styles);
}

const ResourceId& CSSDocument::getResourceId() const {
    return _resourceId;
}

Result<Ref<CSSDocument>> CSSDocument::parse(const ResourceId& resourceId,
                                            const Byte* data,
                                            size_t len,
                                            AttributeIds& attributeIds) {
    Valdi::StyleNode styleNodeRoot;

    bool parsed = styleNodeRoot.ParseFromArray(data, static_cast<int>(len));
    if (!parsed) {
        return Error("Failed to parse CSS document");
    }

    return Valdi::makeShared<CSSDocument>(resourceId, styleNodeRoot, attributeIds);
}

VALDI_CLASS_IMPL(CSSDocument)

} // namespace Valdi
