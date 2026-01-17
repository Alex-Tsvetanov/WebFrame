//
//  ContextAttachedValdiObject.cpp
//  valdi
//
//  Created by Simon Corsin on 5/17/21.
//

#include "valdi/runtime/Context/ContextAttachedValdiObject.hpp"

namespace Valdi {

ContextAttachedObject::ContextAttachedObject(Ref<Context>&& context, const Ref<RefCountable>& innerObject)
    : _context(std::move(context)), _innerObject(innerObject) {
    SC_ASSERT(_context != nullptr, "No context set when creating the ContextAttachedObject");
    _context->insertDisposable(this);
}

ContextAttachedObject::~ContextAttachedObject() {
    _context->removeDisposable(this);
}

bool ContextAttachedObject::dispose(std::unique_lock<Mutex>& disposablesLock) {
    auto innerObject = std::move(_innerObject);
    disposablesLock.unlock();
    return innerObject != nullptr;
}

const Ref<Context>& ContextAttachedObject::getContext() const {
    return _context;
}

Ref<RefCountable> ContextAttachedObject::getInnerObject() const {
    auto lock = _context->lock();
    return _innerObject;
}

Ref<RefCountable> ContextAttachedObject::removeInnerObject() {
    auto lock = _context->lock();
    return std::move(_innerObject);
}

const Ref<RefCountable>& ContextAttachedObject::unsafeGetInnerObject() const {
    return _innerObject;
}

ContextAttachedValdiObject::ContextAttachedValdiObject(Ref<Context>&& context, const Ref<RefCountable>& innerObject)
    : ContextAttachedObject(std::move(context), innerObject) {}

Ref<ValdiObject> ContextAttachedValdiObject::getInnerValdiObject() const {
    return castOrNull<ValdiObject>(getInnerObject());
}

} // namespace Valdi
