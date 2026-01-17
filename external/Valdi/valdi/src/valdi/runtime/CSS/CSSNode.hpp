//
//  ValdiCSSNode.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/22/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include "valdi/runtime/Attributes/Animator.hpp"
#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi/runtime/Attributes/AttributeOwner.hpp"
#include "valdi/runtime/CSS/CSSDocument.hpp"
#include "valdi/runtime/CSS/CSSNodeParentResolver.hpp"
#include "valdi/valdi.pb.h"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class AttributesApplier;
class ViewTransactionScope;

class CSSNode : public AttributeOwner {
public:
    CSSNode();
    CSSNode(const CSSNode&) = delete;
    ~CSSNode() override;

    bool setClass(const StringBox& cssClass);
    const StringBox& getClass() const;

    void applyCss(ViewTransactionScope& viewTransactionScope,
                  const CSSDocument& cssDocument,
                  AttributesApplier& attributesApplier,
                  const SharedAnimator& animator);

    void setParentResolver(CSSNodeParentResolver* parentResolver);

    int getIndexAmongSiblings() const;
    bool setIndexAmongSiblings(int index);

    bool setSiblingsCount(int count);

    bool attributeChanged(AttributeId attribute);

    bool setTagName(const StringBox& tagName);

    bool setNodeId(const StringBox& nodeId);
    const StringBox& getNodeId() const;

    void setMonitoredCssAttributes(MonitoredCssAttributesPtr monitoredCssAttributes);

    const StringBox& getTagName() const;

    int getAttributePriority(AttributeId attributeId) const override;
    void setIsManagingRootOfChildTree(bool managingRootOfChildTree);

    StringBox getAttributeSource(AttributeId attributeId) const override;

private:
    StringBox _tagName;
    StringBox _nodeId;
    CSSNodeParentResolver* _parentResolver = nullptr;
    MonitoredCssAttributesPtr _monitoredCssAttributes;

    FlatSet<StringBox> _resolvedCssClasses;
    StringBox _cssClass;

    std::unique_ptr<FlatMap<AttributeId, const CSSStyleDeclaration*>> _lastStyleDeclarations;

    int _indexAmongSiblings = 0;
    int _siblingsCount = 0;
    bool _managingRootOfChildTree = false;

    void insertDeclarations(const CSSDocument& cssDocument,
                            const CSSStyleNode& styleNode,
                            AttributesApplier& attributesApplier,
                            FlatMap<AttributeId, const CSSStyleDeclaration*>& bestStyleDeclarationByKey);
    void insertDeclarations(const CSSDocument& cssDocument,
                            const CSSProcessedRuleIndex& ruleIndex,
                            AttributesApplier& attributesApplier,
                            FlatMap<AttributeId, const CSSStyleDeclaration*>& bestStyleDeclarationByKey);

    void setAttributeFromDeclaration(ViewTransactionScope& viewTransactionScope,
                                     AttributeId attributeId,
                                     AttributesApplier& attributesApplier,
                                     const CSSStyleDeclaration* styleDeclaration,
                                     const SharedAnimator& animator);

    bool isMonitoredAttribute(AttributeId attribute) const;

    CSSNode* resolveParent(const CSSDocument& cssDocument) const;

    // Style declaration pooling

    static std::unique_ptr<FlatMap<AttributeId, const CSSStyleDeclaration*>> newStyleDeclarationMap();

    static void releaseStyleDeclarationMap(
        std::unique_ptr<FlatMap<AttributeId, const CSSStyleDeclaration*>> styleDeclarationMap);
};

} // namespace Valdi
