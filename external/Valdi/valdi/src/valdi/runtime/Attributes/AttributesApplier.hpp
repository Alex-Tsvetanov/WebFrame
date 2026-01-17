//
//  AttributesApplier.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/13/19.
//

#pragma once

#include "valdi/runtime/Attributes/Animator.hpp"
#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include <optional>

namespace Valdi {

class AttributeOwner;
class ViewTransactionScope;

class AttributesApplier : public SimpleRefCountable {
public:
    /**
     Set the attribute value for the given name and owner, animate the change with the given animator
     if provided. Returns whether the resolved attribute value has changed.
     */
    virtual bool setAttribute(ViewTransactionScope& viewTransactionScope,
                              AttributeId id,
                              const AttributeOwner* owner,
                              const Value& value,
                              const Ref<Animator>& animator) = 0;

    /**
     Remove the attribute value for the given name and owner, animate the change with the given animator
     if provided. Returns whether the attribute value has changed.
     */
    bool removeAttribute(ViewTransactionScope& viewTransactionScope,
                         AttributeId id,
                         const AttributeOwner* owner,
                         const Ref<Animator>& animator) {
        return setAttribute(viewTransactionScope, id, owner, Value::undefined(), animator);
    }

    /**
     Checks if a resolved attribute value for the given name is available.
     Returns false if it is not set.
     */
    virtual bool hasResolvedAttributeValue(AttributeId id) const = 0;

    /**
     Return the resolved attribute value for the given name.
     Returns undefined if it is not set.
     */
    virtual Value getResolvedAttributeValue(AttributeId id) const = 0;

    /**
     Remove all attributes set by the given owner.
     */
    virtual bool removeAllAttributesForOwner(ViewTransactionScope& viewTransactionScope,
                                             const AttributeOwner* owner,
                                             const Ref<Animator>& animator) = 0;

    /**
     Reapply a previously applied attribute, does nothing if the attribute was not applied
     */
    virtual void reapplyAttribute(ViewTransactionScope& viewTransactionScope, AttributeId id) = 0;

    /**
     Apply any pending changes. Mostly used for composite attributes, since they need to be
     calculated from a set of attributes.
     */
    virtual void flush(ViewTransactionScope& viewTransactionScope) = 0;

    /**
     Whether a flush() is expected to be called in order the flush out any pending attributes.
     */
    virtual bool needsFlush() const = 0;
};

} // namespace Valdi
