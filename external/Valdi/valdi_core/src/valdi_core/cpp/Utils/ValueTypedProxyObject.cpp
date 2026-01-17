//
//  ProxyObject.cpp
//  valdi_core
//
//  Created by Simon Corsin on 2/6/23.
//

#include "valdi_core/cpp/Utils/ValueTypedProxyObject.hpp"

namespace Valdi {

ValueTypedProxyObject::ValueTypedProxyObject(const Ref<ValueTypedObject>& typedObject)
    : _id(ValueTypedProxyObject::newId()), _typedObject(typedObject) {}
ValueTypedProxyObject::~ValueTypedProxyObject() = default;

const Ref<ValueTypedObject>& ValueTypedProxyObject::getTypedObject() const {
    return _typedObject;
}

uint32_t ValueTypedProxyObject::getId() const {
    return _id;
}

uint32_t ValueTypedProxyObject::newId() {
    static std::atomic_uint32_t kSequence = 0;

    return ++kSequence;
}

} // namespace Valdi
