//
//  CSSAttributesManager.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/31/19.
//

#pragma once

#include "valdi/runtime/CSS/CSSNodeParentResolver.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi/runtime/Attributes/AttributeIds.hpp"

namespace Valdi {

class AttributesApplier;
class CSSAttributes;
class Animator;
class ILogger;
class AttributeOwner;
class CSSDocument;
class ViewTransactionScope;

struct CSSNodeContainer;

struct CSSAttributesManagerUpdateContext {
    ViewTransactionScope& viewTransactionScope;
    AttributesApplier& attributesApplier;
    [[maybe_unused]] ILogger& logger;
    const Ref<Animator>& animator;

    constexpr CSSAttributesManagerUpdateContext(ViewTransactionScope& viewTransactionScope,
                                                AttributesApplier& attributesApplier,
                                                ILogger& logger,
                                                const Ref<Animator>& animator)
        : viewTransactionScope(viewTransactionScope),
          attributesApplier(attributesApplier),
          logger(logger),
          animator(animator) {}
};

/**
 Manages CSS attributes and apply them on an AttributesApplier.
 Unlike CSSNode, CSSAttributesManager only deals with attributes returned
 from CSSDocument's getAttributesForClass().
 */
class CSSAttributesManager : protected CSSNodeParentResolver {
public:
    CSSAttributesManager();
    ~CSSAttributesManager() override;

    /**
     Apply the style given the weakly typed style value.
     The value can be an array of style or a style object.
     Returns whether there were any changes to the underlying attributes.
     */
    bool apply(const Value& style, const CSSAttributesManagerUpdateContext& context);

    /**
     Do a CSS update pass using the given animator.
     */
    void updateCSS(const CSSAttributesManagerUpdateContext& context);

    Shared<CSSDocument> getCSSDocument() const;

    bool setCSSDocument(const Value& cssDocument, bool isOverridenFromParent);
    bool setCSSClass(const Value& cssClass, bool isOverridenFromParent);
    bool setElementId(const StringBox& elementId, bool isOverridenFromParent);
    bool setElementTag(const StringBox& elementTag, bool isOverridenFromParent);
    bool setSiblingsIndexes(int siblingsCount, int indexAmongSiblings);

    bool attributeChanged(AttributeId attribute);

    StringBox getNodeId() const;
    bool hasNodeId(const StringBox& nodeId) const;

    bool needUpdateCSS() const;

    void setParent(CSSAttributesManager* attributesManagerOfParent);

    void copyCSSDocument(const CSSAttributesManager& other);

protected:
    CSSNode* getCSSParentForDocument(const CSSDocument& cssDocument) override;

private:
    SmallVector<Ref<CSSAttributes>, 2> _styles;
    std::unique_ptr<CSSNodeContainer> _cssNodeContainer;
    std::unique_ptr<CSSNodeContainer> _cssNodeContainerFromParentOveridde;
    CSSAttributesManager* _attributesManagerOfParent = nullptr;

    bool removeAllStyles(const CSSAttributesManagerUpdateContext& context);

    bool setStyles(SmallVector<Ref<CSSAttributes>, 2> styles, const CSSAttributesManagerUpdateContext& context);

    static bool containsStyle(const SmallVector<Ref<CSSAttributes>, 2>& styles, const Ref<CSSAttributes>& style);

    static void applyAttributesOfStyle(const Ref<CSSAttributes>& cssAttributes,
                                       const CSSAttributesManagerUpdateContext& context,
                                       bool& changed);
    static void removeAttributesOfStyle(const Ref<CSSAttributes>& cssAttributes,
                                        const CSSAttributesManagerUpdateContext& context,
                                        bool& changed);

    CSSNodeContainer& getCSSNodeContainer(bool isOverridenFromParent);
    CSSNodeContainer* getCSSNodeContainerForDocument(const CSSDocument* document) const;
};

} // namespace Valdi
