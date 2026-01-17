//
//  CSSAttributesManager.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/31/19.
//

#include "valdi/runtime/CSS/CSSAttributesManager.hpp"
#include "valdi/runtime/Attributes/AttributesApplier.hpp"
#include "valdi/runtime/CSS/CSSAttributes.hpp"
#include "valdi/runtime/CSS/CSSDocument.hpp"
#include "valdi/runtime/CSS/CSSNode.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/ObjectPool.hpp"

namespace Valdi {

struct CSSNodeContainer {
    CSSNode node;
    Ref<CSSDocument> cssDocument;
};

CSSAttributesManager::CSSAttributesManager() = default;

CSSAttributesManager::~CSSAttributesManager() = default;

bool CSSAttributesManager::apply(const Value& style, const CSSAttributesManagerUpdateContext& context) {
    if (style.isNullOrUndefined()) {
        return removeAllStyles(context);
    } else if (style.isArray()) {
        SmallVector<Ref<CSSAttributes>, 2> styles;

        [[maybe_unused]] size_t index = 0;
        for (const auto& arrayItem : *style.getArray()) {
            auto cssAttributes = castOrNull<CSSAttributes>(arrayItem.getValdiObject());
            if (cssAttributes == nullptr) {
                VALDI_ERROR(context.logger,
                            "Failed to apply style: Given style {} at index {} is not a CSSAttributes object",
                            style.toString(),
                            index);
            } else {
                styles.emplace_back(std::move(cssAttributes));
            }
            index++;
        }

        return setStyles(std::move(styles), context);
    } else {
        auto cssAttributes = castOrNull<CSSAttributes>(style.getValdiObject());
        if (cssAttributes == nullptr) {
            VALDI_ERROR(context.logger,
                        "Failed to apply style: Given style {} is not a CSSAttributes object",
                        style.toString());
            return removeAllStyles(context);
        }

        SmallVector<Ref<CSSAttributes>, 2> styles;
        styles.emplace_back(std::move(cssAttributes));

        return setStyles(std::move(styles), context);
    }
}

bool CSSAttributesManager::removeAllStyles(const CSSAttributesManagerUpdateContext& context) {
    auto changed = false;

    while (!_styles.empty()) {
        auto style = std::move(_styles[_styles.size() - 1]);
        _styles.pop_back();

        if (context.attributesApplier.removeAllAttributesForOwner(
                context.viewTransactionScope, style.get(), context.animator)) {
            changed = true;
        }
    }

    return changed;
}

bool CSSAttributesManager::setStyles(SmallVector<Ref<CSSAttributes>, 2> styles,
                                     const CSSAttributesManagerUpdateContext& context) {
    auto changed = false;

    auto stylesToRemove = makeReusableArray<Ref<CSSAttributes>>();

    // Step 1: Find all styles which are not present anymore
    auto it = _styles.begin();
    while (it != _styles.end()) {
        if (containsStyle(styles, *it)) {
            it++;
        } else {
            auto style = std::move(*it);
            it = _styles.erase(it);
            stylesToRemove->emplace_back(std::move(style));
        }
    }

    // Step 2: Apply the new styles
    for (const auto& style : styles) {
        if (!containsStyle(_styles, style)) {
            applyAttributesOfStyle(style, context, changed);
        }
    }

    // Step 3: Remove the styles which have been previously found
    for (const auto& style : *stylesToRemove) {
        removeAttributesOfStyle(style, context, changed);
    }

    _styles = std::move(styles);

    return changed;
}

bool CSSAttributesManager::containsStyle(const SmallVector<Ref<CSSAttributes>, 2>& styles,
                                         const Ref<CSSAttributes>& style) {
    return std::any_of(styles.begin(), styles.end(), [&style](const Ref<CSSAttributes>& existingStyle) {
        return existingStyle == style;
    });
}

void CSSAttributesManager::removeAttributesOfStyle(const Ref<CSSAttributes>& cssAttributes,
                                                   const CSSAttributesManagerUpdateContext& context,
                                                   bool& changed) {
    if (context.attributesApplier.removeAllAttributesForOwner(
            context.viewTransactionScope, cssAttributes.get(), context.animator)) {
        changed = true;
    }
}

void CSSAttributesManager::applyAttributesOfStyle(const Ref<CSSAttributes>& cssAttributes,
                                                  const CSSAttributesManagerUpdateContext& context,
                                                  bool& changed) {
    for (const auto& style : cssAttributes->getStyles()) {
        if (context.attributesApplier.setAttribute(context.viewTransactionScope,
                                                   style.attribute.id,
                                                   cssAttributes.get(),
                                                   style.attribute.value,
                                                   context.animator)) {
            changed = true;
        }
    }
}

void CSSAttributesManager::updateCSS(const CSSAttributesManagerUpdateContext& context) {
    if (_cssNodeContainerFromParentOveridde != nullptr && _cssNodeContainerFromParentOveridde->cssDocument != nullptr) {
        _cssNodeContainerFromParentOveridde->node.applyCss(context.viewTransactionScope,
                                                           *_cssNodeContainerFromParentOveridde->cssDocument,
                                                           context.attributesApplier,
                                                           context.animator);
    }
    if (_cssNodeContainer != nullptr && _cssNodeContainer->cssDocument != nullptr) {
        _cssNodeContainer->node.applyCss(
            context.viewTransactionScope, *_cssNodeContainer->cssDocument, context.attributesApplier, context.animator);
    }
}

bool CSSAttributesManager::setCSSClass(const Value& cssClass, bool isOverridenFromParent) {
    return getCSSNodeContainer(isOverridenFromParent).node.setClass(cssClass.toStringBox());
}

bool CSSAttributesManager::setCSSDocument(const Value& cssDocument, bool isOverridenFromParent) {
    auto cssDocumentNative = castOrNull<CSSDocument>(cssDocument.getValdiObject());

    auto& nodeContainer = getCSSNodeContainer(isOverridenFromParent);

    if (cssDocumentNative == nodeContainer.cssDocument) {
        return false;
    }

    nodeContainer.cssDocument = std::move(cssDocumentNative);
    if (nodeContainer.cssDocument != nullptr) {
        nodeContainer.node.setMonitoredCssAttributes(nodeContainer.cssDocument->getMonitoredAttributes());
    } else {
        nodeContainer.node.setMonitoredCssAttributes(nullptr);
    }

    return true;
}

bool CSSAttributesManager::setElementTag(const StringBox& elementTag, bool isOverridenFromParent) {
    return getCSSNodeContainer(isOverridenFromParent).node.setTagName(elementTag);
}

bool CSSAttributesManager::setElementId(const StringBox& elementId, bool isOverridenFromParent) {
    return getCSSNodeContainer(isOverridenFromParent).node.setNodeId(elementId);
}

CSSNodeContainer* CSSAttributesManager::getCSSNodeContainerForDocument(const CSSDocument* cssDocument) const {
    if (_cssNodeContainer != nullptr && _cssNodeContainer->cssDocument.get() == cssDocument) {
        return _cssNodeContainer.get();
    }
    if (_cssNodeContainerFromParentOveridde != nullptr &&
        _cssNodeContainerFromParentOveridde->cssDocument.get() == cssDocument) {
        return _cssNodeContainerFromParentOveridde.get();
    }
    return nullptr;
}

CSSNode* CSSAttributesManager::getCSSParentForDocument(const CSSDocument& cssDocument) {
    if (_attributesManagerOfParent != nullptr) {
        auto* nodeContainer = _attributesManagerOfParent->getCSSNodeContainerForDocument(&cssDocument);
        if (nodeContainer != nullptr) {
            return &nodeContainer->node;
        }
        return _attributesManagerOfParent->getCSSParentForDocument(cssDocument);
    }

    return nullptr;
}

void CSSAttributesManager::copyCSSDocument(const CSSAttributesManager& other) {
    if (other.getCSSDocument() != nullptr) {
        setCSSDocument(Value(other.getCSSDocument()), false);
    }
}

void CSSAttributesManager::setParent(CSSAttributesManager* attributesManagerOfParent) {
    _attributesManagerOfParent = attributesManagerOfParent;
}

bool CSSAttributesManager::setSiblingsIndexes(int siblingsCount, int indexAmongSiblings) {
    auto changed = false;

    if (_cssNodeContainer != nullptr) {
        changed |= _cssNodeContainer->node.setSiblingsCount(siblingsCount);
        changed |= _cssNodeContainer->node.setIndexAmongSiblings(indexAmongSiblings);
    }
    if (_cssNodeContainerFromParentOveridde != nullptr) {
        changed |= _cssNodeContainerFromParentOveridde->node.setSiblingsCount(siblingsCount);
        changed |= _cssNodeContainerFromParentOveridde->node.setIndexAmongSiblings(indexAmongSiblings);
    }

    return changed;
}

StringBox CSSAttributesManager::getNodeId() const {
    if (_cssNodeContainer != nullptr) {
        return _cssNodeContainer->node.getNodeId();
    }

    return StringBox();
}

Shared<CSSDocument> CSSAttributesManager::getCSSDocument() const {
    if (_cssNodeContainer != nullptr) {
        return _cssNodeContainer->cssDocument.toShared();
    }

    return nullptr;
}

bool CSSAttributesManager::needUpdateCSS() const {
    return _cssNodeContainer != nullptr || _cssNodeContainerFromParentOveridde != nullptr;
}

bool CSSAttributesManager::hasNodeId(const StringBox& nodeId) const {
    if (_cssNodeContainer != nullptr && _cssNodeContainer->node.getNodeId() == nodeId) {
        return true;
    }
    if (_cssNodeContainerFromParentOveridde != nullptr &&
        _cssNodeContainerFromParentOveridde->node.getNodeId() == nodeId) {
        return true;
    }

    return false;
}

bool CSSAttributesManager::attributeChanged(AttributeId attribute) {
    bool changed = false;
    if (_cssNodeContainer != nullptr) {
        changed |= _cssNodeContainer->node.attributeChanged(attribute);
    }
    if (_cssNodeContainerFromParentOveridde != nullptr) {
        changed |= _cssNodeContainerFromParentOveridde->node.attributeChanged(attribute);
    }
    return changed;
}

CSSNodeContainer& CSSAttributesManager::getCSSNodeContainer(bool isOverridenFromParent) {
    if (isOverridenFromParent) {
        if (_cssNodeContainerFromParentOveridde == nullptr) {
            _cssNodeContainerFromParentOveridde = std::make_unique<CSSNodeContainer>();
            _cssNodeContainerFromParentOveridde->node.setParentResolver(this);
            _cssNodeContainerFromParentOveridde->node.setIsManagingRootOfChildTree(true);
        }
        return *_cssNodeContainerFromParentOveridde;
    } else {
        if (_cssNodeContainer == nullptr) {
            _cssNodeContainer = std::make_unique<CSSNodeContainer>();
            _cssNodeContainer->node.setParentResolver(this);
        }
        return *_cssNodeContainer;
    }
}

} // namespace Valdi
