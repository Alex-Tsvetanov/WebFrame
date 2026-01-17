//
//  ProxyObject.hpp
//  valdi_core
//
//  Created by Simon Corsin on 2/6/23.
//

#pragma once

#include "valdi_core/cpp/Utils/ValueTypedObject.hpp"

namespace Valdi {

/**
 A typed proxy object is a marshalled persistent representation
 of a platform object that has a typed object representation.
 It  is backed by an instance from a foreign platform, for example Objective-C or JS.
 The typed object that the proxy holds is the cross-platform, typed representation of the proxy.

 Proxy objects are created by the ValueMarshallerRegistry when processing protocol class schemas.
 */
class ValueTypedProxyObject : public SharedPtrRefCountable {
public:
    explicit ValueTypedProxyObject(const Ref<ValueTypedObject>& typedObject);
    ~ValueTypedProxyObject() override;

    const Ref<ValueTypedObject>& getTypedObject() const;

    virtual std::string_view getType() const = 0;

    uint32_t getId() const;

    virtual bool expired() const {
        return false;
    }

private:
    uint32_t _id;
    Ref<ValueTypedObject> _typedObject;

    static uint32_t newId();
};

} // namespace Valdi
