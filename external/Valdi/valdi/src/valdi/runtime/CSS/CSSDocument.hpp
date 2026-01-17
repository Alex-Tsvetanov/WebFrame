//
//  CSSDocument.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 6/26/19.
//

#pragma once

#include "valdi/runtime/CSS/CSSAttributes.hpp"
#include "valdi/valdi.pb.h"
#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include "valdi/runtime/Attributes/AttributeIds.hpp"

#include <vector>

namespace Valdi {

struct CSSProcessedRuleIndex;

struct CSSStyleNode {
    std::vector<CSSStyleDeclaration> styles;
    std::unique_ptr<CSSProcessedRuleIndex> ruleIndex = nullptr;
};

struct CSSAttributeRule {
    CSSAttribute attribute;
    CSSStyleNode styleNode;
    Valdi::CSSRuleIndex_AttributeRule_Type type;
};

struct CSSNthChildRule {
    CSSStyleNode node;
    int n = 0;
    int offset = 0;
};

struct CSSProcessedRuleIndex {
    FlatMap<StringBox, CSSStyleNode> idRules;
    FlatMap<StringBox, CSSStyleNode> classRules;
    FlatMap<StringBox, CSSStyleNode> tagRules;
    std::vector<CSSAttributeRule> attributeRules;
    std::unique_ptr<CSSStyleNode> firstChildRule;
    std::unique_ptr<CSSStyleNode> lastChildRule;
    std::vector<CSSNthChildRule> nthChildRules;
    std::unique_ptr<CSSProcessedRuleIndex> ancestorRules;
    std::unique_ptr<CSSProcessedRuleIndex> directParentRules;
};

using MonitoredCssAttributesPtr = std::shared_ptr<FlatSet<AttributeId>>;

class CSSDocument : public ValdiObject {
public:
    CSSDocument(const ResourceId& resourceId, const Valdi::StyleNode& styleNode, AttributeIds& attributeIds);
    ~CSSDocument() override;

    const CSSStyleNode& getRootNode() const;
    const MonitoredCssAttributesPtr& getMonitoredAttributes() const;

    Result<Ref<CSSAttributes>> getAttributesForClass(const StringBox& className) const;

    static Result<Ref<CSSDocument>> parse(const ResourceId& resourceId,
                                          const Byte* data,
                                          size_t len,
                                          AttributeIds& attributeIds);

    const ResourceId& getResourceId() const;

    VALDI_CLASS_HEADER(CSSDocument)
private:
    ResourceId _resourceId;
    CSSStyleNode _rootNode;
    MonitoredCssAttributesPtr _monitoredCssAttributes;

    void populateStyleNode(AttributeIds& attributeIds, const Valdi::StyleNode& styleNode, CSSStyleNode& currentNode);
    void populateRuleIndex(AttributeIds& attributeIds,
                           const Valdi::CSSRuleIndex& ruleIndex,
                           CSSProcessedRuleIndex& currentRuleIndex);
    static void populateStyleDeclaration(AttributeIds& attributeIds,
                                         const Valdi::StyleDeclaration& styleDeclaration,
                                         CSSStyleDeclaration& currentStyleDeclaration);

    void populateMapRule(AttributeIds& attributeIds,
                         const google::protobuf::RepeatedPtrField<::Valdi::NamedStyleNode>& mapRule,
                         FlatMap<StringBox, CSSStyleNode>& currentMap);
};

} // namespace Valdi
