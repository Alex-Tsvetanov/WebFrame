//
//  BoundAttributes.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 7/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeHandler.hpp"
#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <memory>
#include <string>

namespace Valdi {

class AttributeHandlerDelegate;
class MeasureDelegate;

class BoundAttributes : public SharedPtrRefCountable {
public:
    BoundAttributes(const StringBox& className,
                    AttributeHandlerById handlerByAttributeName,
                    const Ref<AttributeHandlerDelegate>& defaultAttributeHandlerDelegate,
                    const Ref<MeasureDelegate>& measureDelegate,
                    AttributeIds& attributeIds,
                    bool scrollable);
    ~BoundAttributes() override;

    AttributeHandler* getAttributeHandlerForId(AttributeId attributeId);

    const StringBox& getClassName() const;

    const AttributeHandlerById& getHandlers() const;

    bool isBackingClassScrollable() const;

    AttributeIds& getAttributeIds() const;
    const Ref<AttributeHandlerDelegate>& getDefaultAttributeHandlerDelegate() const;
    const Ref<MeasureDelegate>& getMeasureDelegate() const;

    Ref<BoundAttributes> merge(const StringBox& className,
                               const AttributeHandlerById& handlerByAttributeName,
                               const Ref<AttributeHandlerDelegate>& defaultAttributeHandlerDelegate,
                               const Ref<MeasureDelegate>& measureDelegate,
                               bool scrollable,
                               bool allowOverride) const;

private:
    StringBox _className;
    AttributeHandlerById _handlerByAttributeName;
    Ref<AttributeHandlerDelegate> _defaultAttributeHandlerDelegate;
    Ref<MeasureDelegate> _measureDelegate;
    AttributeIds& _attributeIds;
    bool _scrollable;
    std::vector<AttributeHandler*> _attributeHandlers;
    std::unique_ptr<std::vector<Shared<AttributeHandler>>> _allocatedDefaultAttributeHandlers;

    AttributeHandler* getDefaultAttributeHandlerForId(AttributeId attributeId);

    void insertAttributeHandler(AttributeHandler* attributeHandler);
};

using SharedBoundAttributes = Ref<BoundAttributes>;

} // namespace Valdi
