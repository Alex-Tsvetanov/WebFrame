//
//  ValdiCSSNode.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/22/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/runtime/CSS/CSSNode.hpp"
#include "valdi/runtime/Attributes/AttributesApplier.hpp"
#include "valdi/runtime/CSS/CSSDocument.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>

namespace Valdi {

template<typename T>
void forEachCSSClass(const StringBox& cssClasses, T&& callback) {
    size_t rangeStart = 0;
    size_t i = 0;
    auto length = cssClasses.length();

    const char* str = cssClasses.getCStr();

    while (i < length) {
        if (*str == ' ') {
            // Ignore empty range.
            if (rangeStart != i) {
                auto subStr =
                    Valdi::StringCache::getGlobal().makeString(cssClasses.getCStr() + rangeStart, i - rangeStart);
                callback(subStr);
            }

            rangeStart = i + 1;
        }

        str++;
        i++;
    }

    if (rangeStart != i) {
        if (rangeStart == 0) {
            // We had no multiple, pass the original string
            callback(cssClasses);
        } else {
            auto subStr = Valdi::StringCache::getGlobal().makeString(cssClasses.getCStr() + rangeStart, i - rangeStart);
            callback(subStr);
        }
    }
}

CSSNode::CSSNode() = default;

CSSNode::~CSSNode() {
    if (_lastStyleDeclarations != nullptr) {
        CSSNode::releaseStyleDeclarationMap(std::move(_lastStyleDeclarations));
    }
}

bool CSSNode::setClass(const StringBox& cssClass) {
    if (_cssClass == cssClass) {
        return false;
    }

    _cssClass = cssClass;
    _resolvedCssClasses.clear();

    forEachCSSClass(cssClass, [&](StringBox cssClass) { _resolvedCssClasses.emplace(std::move(cssClass)); });

    return true;
}

bool CSSNode::setNodeId(const StringBox& nodeId) {
    if (_nodeId == nodeId) {
        return false;
    }
    _nodeId = nodeId;
    return true;
}

const StringBox& CSSNode::getNodeId() const {
    return _nodeId;
}

bool CSSNode::setTagName(const StringBox& tagName) {
    if (_tagName == tagName) {
        return false;
    }
    _tagName = tagName;
    return true;
}

void CSSNode::setIsManagingRootOfChildTree(bool managingRootOfChildTree) {
    _managingRootOfChildTree = managingRootOfChildTree;
}

void CSSNode::setMonitoredCssAttributes(MonitoredCssAttributesPtr monitoredCssAttributes) {
    _monitoredCssAttributes = std::move(monitoredCssAttributes);
}

const StringBox& CSSNode::getClass() const {
    return _cssClass;
}

bool CSSNode::isMonitoredAttribute(AttributeId attribute) const {
    if (_monitoredCssAttributes == nullptr) {
        return false;
    }

    return _monitoredCssAttributes->find(attribute) != _monitoredCssAttributes->end();
}

bool CSSNode::attributeChanged(AttributeId attribute) {
    return isMonitoredAttribute(attribute);
}

int CSSNode::getIndexAmongSiblings() const {
    return _indexAmongSiblings;
}

bool CSSNode::setIndexAmongSiblings(int index) {
    if (index == _indexAmongSiblings) {
        return false;
    }

    _indexAmongSiblings = index;
    return true;
}

bool CSSNode::setSiblingsCount(int count) {
    if (count == _siblingsCount) {
        return false;
    }

    _siblingsCount = count;
    return true;
}

inline bool shouldReplaceDeclaration(const CSSStyleDeclaration& existing, const CSSStyleDeclaration& newDeclaration) {
    if (newDeclaration.priority < existing.priority) {
        return false;
    }
    if (newDeclaration.priority > existing.priority) {
        return true;
    }

    return newDeclaration.order > existing.order;
}

void insertDeclarationIfNeeded(const CSSStyleDeclaration& declaration,
                               FlatMap<AttributeId, const CSSStyleDeclaration*>& bestStyleDeclarationByKey) {
    auto attributeId = declaration.attribute.id;

    auto it = bestStyleDeclarationByKey.try_emplace(attributeId, &declaration);
    if (!it.second) {
        if (shouldReplaceDeclaration(*it.first->second, declaration)) {
            it.first->second = &declaration;
        }
    }
}

void CSSNode::insertDeclarations(const CSSDocument& cssDocument,
                                 const CSSStyleNode& styleNode,
                                 AttributesApplier& attributesApplier,
                                 FlatMap<AttributeId, const CSSStyleDeclaration*>& bestStyleDeclarationByKey) {
    for (const auto& decl : styleNode.styles) {
        insertDeclarationIfNeeded(decl, bestStyleDeclarationByKey);
    }

    if (styleNode.ruleIndex != nullptr) {
        insertDeclarations(cssDocument, *styleNode.ruleIndex, attributesApplier, bestStyleDeclarationByKey);
    }
}

inline bool nthChildMatchesIndex(const int index, const int n, const int offset) {
    // nth-child element indexes start at one
    auto adjustedIndex = index + 1;
    if (n == 0) {
        return adjustedIndex == offset;
    }

    auto offsetIndex = adjustedIndex - offset;
    return offsetIndex % n == 0 && offsetIndex / n >= 0;
}

void CSSNode::insertDeclarations(const CSSDocument& cssDocument,
                                 const CSSProcessedRuleIndex& ruleIndex,
                                 AttributesApplier& attributesApplier,
                                 FlatMap<AttributeId, const CSSStyleDeclaration*>& bestStyleDeclarationByKey) {
    // ORDER: id -> class -> tag -> attrib -> firstChild -> lastChild -> nthChild

    if (!ruleIndex.idRules.empty()) {
        const auto& idRules = ruleIndex.idRules;

        const auto& childNode = idRules.find(_nodeId);
        if (childNode != idRules.end()) {
            insertDeclarations(cssDocument, childNode->second, attributesApplier, bestStyleDeclarationByKey);
        }
    }

    if (!ruleIndex.classRules.empty()) {
        const auto& classRules = ruleIndex.classRules;
        for (const auto& className : _resolvedCssClasses) {
            auto it = classRules.find(className);
            if (it != classRules.end()) {
                insertDeclarations(cssDocument, it->second, attributesApplier, bestStyleDeclarationByKey);
            }
        }
    }

    if (!ruleIndex.tagRules.empty()) {
        const auto& tagRules = ruleIndex.tagRules;

        if (!_tagName.isEmpty()) {
            auto it = tagRules.find(_tagName);
            if (it != tagRules.end()) {
                insertDeclarations(cssDocument, it->second, attributesApplier, bestStyleDeclarationByKey);
            }
        }

        static auto kWildcardRule = STRING_LITERAL("*");

        auto it = tagRules.find(kWildcardRule);
        if (it != tagRules.end()) {
            insertDeclarations(cssDocument, it->second, attributesApplier, bestStyleDeclarationByKey);
        }
    }

    if (!ruleIndex.attributeRules.empty()) {
        const auto& attributeRules = ruleIndex.attributeRules;

        for (const auto& attributeRule : attributeRules) {
            switch (attributeRule.type) {
                case Valdi::CSSRuleIndex_AttributeRule_Type_EQUALS: {
                    auto value = attributesApplier.getResolvedAttributeValue(attributeRule.attribute.id);
                    if (value == attributeRule.attribute.value) {
                        insertDeclarations(
                            cssDocument, attributeRule.styleNode, attributesApplier, bestStyleDeclarationByKey);
                    }
                } break;
                default:
                    // TODO: (nate) implement
                    break;
            }
        }
    }

    if (ruleIndex.firstChildRule != nullptr) {
        if (_indexAmongSiblings == 0) {
            insertDeclarations(cssDocument, *ruleIndex.firstChildRule, attributesApplier, bestStyleDeclarationByKey);
        }
    }

    if (ruleIndex.lastChildRule != nullptr) {
        if (_indexAmongSiblings == _siblingsCount - 1) {
            insertDeclarations(cssDocument, *ruleIndex.lastChildRule, attributesApplier, bestStyleDeclarationByKey);
        }
    }

    if (!ruleIndex.nthChildRules.empty()) {
        const auto& nthChildRules = ruleIndex.nthChildRules;
        for (const auto& nthChildRule : nthChildRules) {
            if (nthChildMatchesIndex(_indexAmongSiblings, nthChildRule.n, nthChildRule.offset)) {
                insertDeclarations(cssDocument, nthChildRule.node, attributesApplier, bestStyleDeclarationByKey);
            }
        }
    }

    if (ruleIndex.directParentRules != nullptr) {
        auto* parent = resolveParent(cssDocument);
        if (parent != nullptr) {
            parent->insertDeclarations(
                cssDocument, *ruleIndex.directParentRules, attributesApplier, bestStyleDeclarationByKey);
        }
    }

    if (ruleIndex.ancestorRules != nullptr) {
        auto* ancestor = resolveParent(cssDocument);
        while (ancestor != nullptr) {
            ancestor->insertDeclarations(
                cssDocument, *ruleIndex.ancestorRules, attributesApplier, bestStyleDeclarationByKey);
            ancestor = ancestor->resolveParent(cssDocument);
        }
    }
}

void CSSNode::setAttributeFromDeclaration(ViewTransactionScope& viewTransactionScope,
                                          AttributeId attributeId,
                                          AttributesApplier& attributesApplier,
                                          const CSSStyleDeclaration* styleDeclaration,
                                          const SharedAnimator& animator) {
    attributesApplier.setAttribute(
        viewTransactionScope, attributeId, this, styleDeclaration->attribute.value, animator);
}

void CSSNode::applyCss(ViewTransactionScope& viewTransactionScope,
                       const CSSDocument& cssDocument,
                       AttributesApplier& attributesApplier,
                       const SharedAnimator& animator) {
    auto bestStyleDeclarationByKey = CSSNode::newStyleDeclarationMap();

    insertDeclarations(cssDocument, cssDocument.getRootNode(), attributesApplier, *bestStyleDeclarationByKey);

    if (_lastStyleDeclarations != nullptr) {
        for (const auto& it : *_lastStyleDeclarations) {
            if (bestStyleDeclarationByKey->find(it.first) == bestStyleDeclarationByKey->end()) {
                // If our applied CSS attribute is not among the new CSS attributes
                // we remove it.
                attributesApplier.removeAttribute(viewTransactionScope, it.first, this, animator);
            }
        }
        CSSNode::releaseStyleDeclarationMap(std::move(_lastStyleDeclarations));
    }

    for (const auto& tuple : *bestStyleDeclarationByKey) {
        const auto& property = tuple.first;

        setAttributeFromDeclaration(viewTransactionScope, property, attributesApplier, tuple.second, animator);
    }
    _lastStyleDeclarations = std::move(bestStyleDeclarationByKey);
}

CSSNode* CSSNode::resolveParent(const CSSDocument& cssDocument) const {
    if (_parentResolver == nullptr) {
        return nullptr;
    }

    return _parentResolver->getCSSParentForDocument(cssDocument);
}

void CSSNode::setParentResolver(CSSNodeParentResolver* parentResolver) {
    _parentResolver = parentResolver;
}

int CSSNode::getAttributePriority(AttributeId /*id*/) const {
    if (_managingRootOfChildTree) {
        return kAttributeOwnerPriorityCSSParentOverriden;
    } else {
        return kAttributeOwnerPriorityCSS;
    }
}

StringBox CSSNode::getAttributeSource(AttributeId /*id*/) const {
    static auto kSource = STRING_LITERAL("css");

    return kSource;
}

const StringBox& CSSNode::getTagName() const {
    return _tagName;
}

thread_local std::vector<std::unique_ptr<FlatMap<AttributeId, const CSSStyleDeclaration*>>> styleDeclarationsPool;

std::unique_ptr<FlatMap<AttributeId, const CSSStyleDeclaration*>> CSSNode::newStyleDeclarationMap() {
    auto& mapPool = styleDeclarationsPool;
    if (mapPool.empty()) {
        auto map = std::make_unique<FlatMap<AttributeId, const CSSStyleDeclaration*>>();
        map->reserve(10);
        return map;
    } else {
        auto map = std::move(mapPool[mapPool.size() - 1]);
        mapPool.pop_back();
        return map;
    }
}

void CSSNode::releaseStyleDeclarationMap(
    std::unique_ptr<FlatMap<AttributeId, const CSSStyleDeclaration*>> styleDeclarationMap) {
    styleDeclarationMap->clear();
    styleDeclarationsPool.emplace_back(std::move(styleDeclarationMap));
}

} // namespace Valdi
