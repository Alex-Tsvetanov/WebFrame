//
//  BoundAttributes.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 7/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/runtime/Attributes/BoundAttributes.hpp"
#include "valdi/runtime/Attributes/AttributeHandlerDelegate.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Views/MeasureDelegate.hpp"

namespace Valdi {

BoundAttributes::BoundAttributes(const StringBox& className,
                                 AttributeHandlerById handlerByAttributeName,
                                 const Ref<AttributeHandlerDelegate>& defaultAttributeHandlerDelegate,
                                 const Ref<MeasureDelegate>& measureDelegate,
                                 AttributeIds& attributeIds,
                                 bool scrollable)
    : _className(className),
      _handlerByAttributeName(std::move(handlerByAttributeName)),
      _defaultAttributeHandlerDelegate(defaultAttributeHandlerDelegate),
      _measureDelegate(measureDelegate),
      _attributeIds(attributeIds),
      _scrollable(scrollable) {
    for (auto& it : _handlerByAttributeName) {
        insertAttributeHandler(&it.second);
    }
}

BoundAttributes::~BoundAttributes() = default;

void BoundAttributes::insertAttributeHandler(AttributeHandler* attributeHandler) {
    auto attributeId = attributeHandler->getId();

    while (attributeId >= _attributeHandlers.size()) {
        _attributeHandlers.emplace_back(nullptr);
    }

    _attributeHandlers[attributeId] = attributeHandler;
}

AttributeHandler* BoundAttributes::getAttributeHandlerForId(AttributeId attributeId) {
    if (_defaultAttributeHandlerDelegate != nullptr) {
        if (attributeId >= _attributeHandlers.size() || _attributeHandlers[attributeId] == nullptr) {
            return getDefaultAttributeHandlerForId(attributeId);
        }
    }

    if (attributeId >= _attributeHandlers.size()) {
        return nullptr;
    }

    return _attributeHandlers[attributeId];
}

AttributeHandler* BoundAttributes::getDefaultAttributeHandlerForId(AttributeId attributeId) {
    if (_allocatedDefaultAttributeHandlers == nullptr) {
        _allocatedDefaultAttributeHandlers = std::make_unique<std::vector<Shared<AttributeHandler>>>();
    }

    auto name = _attributeIds.getNameForId(attributeId);

    auto attributeHandler =
        Valdi::makeShared<AttributeHandler>(attributeId, name, _defaultAttributeHandlerDelegate, nullptr, true, false);

    _allocatedDefaultAttributeHandlers->emplace_back(attributeHandler);

    insertAttributeHandler(attributeHandler.get());

    return attributeHandler.get();
}

Ref<BoundAttributes> BoundAttributes::merge(const StringBox& className,
                                            const AttributeHandlerById& handlerByAttributeName,
                                            const Ref<AttributeHandlerDelegate>& defaultAttributeHandlerDelegate,
                                            const Ref<MeasureDelegate>& measureDelegate,
                                            bool scrollable,
                                            bool allowOverride) const {
    auto handlers = _handlerByAttributeName;
    for (const auto& boundAttribute : handlerByAttributeName) {
        if (!allowOverride && handlers.find(boundAttribute.first) != handlers.end()) {
            continue;
        }

        handlers[boundAttribute.first] = boundAttribute.second;
    }

    return Valdi::makeShared<Valdi::BoundAttributes>(
        className,
        std::move(handlers),
        defaultAttributeHandlerDelegate != nullptr ? defaultAttributeHandlerDelegate : _defaultAttributeHandlerDelegate,
        measureDelegate != nullptr ? measureDelegate : _measureDelegate,
        _attributeIds,
        isBackingClassScrollable() || scrollable);
}

const StringBox& BoundAttributes::getClassName() const {
    return _className;
}

bool BoundAttributes::isBackingClassScrollable() const {
    return _scrollable;
}

const AttributeHandlerById& BoundAttributes::getHandlers() const {
    return _handlerByAttributeName;
}

AttributeIds& BoundAttributes::getAttributeIds() const {
    return _attributeIds;
}

const Ref<AttributeHandlerDelegate>& BoundAttributes::getDefaultAttributeHandlerDelegate() const {
    return _defaultAttributeHandlerDelegate;
}

const Ref<MeasureDelegate>& BoundAttributes::getMeasureDelegate() const {
    return _measureDelegate;
}

} // namespace Valdi
