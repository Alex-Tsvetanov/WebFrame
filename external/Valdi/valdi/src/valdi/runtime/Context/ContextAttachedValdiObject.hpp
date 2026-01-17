//
//  ContextAttachedValdiObject.hpp
//  valdi
//
//  Created by Simon Corsin on 5/17/21.
//

#pragma once

#include "valdi/runtime/Context/Context.hpp"
#include "valdi/runtime/Utils/Disposable.hpp"

namespace Valdi {

/**
 A reference to an opaque object that can be disposed.
 */
class ContextAttachedObject : public Disposable {
public:
    ContextAttachedObject(Ref<Context>&& context, const Ref<RefCountable>& object);
    ~ContextAttachedObject() override;

    bool dispose(std::unique_lock<Mutex>& disposablesLock) override;

    const Ref<Context>& getContext() const;
    Ref<RefCountable> getInnerObject() const;

    /**
     Return the inner object, return the reference which was removed
    */
    Ref<RefCountable> removeInnerObject();

    /**
     Return the underlying object without acquiring the Context lock.
     This should be only called for code that already has the Context lock.
     */
    const Ref<RefCountable>& unsafeGetInnerObject() const;

private:
    Ref<Context> _context;
    Ref<RefCountable> _innerObject;
};

/**
 A reference to a ValdiObject that can be disposed.
 */
class ContextAttachedValdiObject : public SimpleRefCountable, public ContextAttachedObject {
public:
    ContextAttachedValdiObject(Ref<Context>&& context, const Ref<RefCountable>& innerObject);

    Ref<ValdiObject> getInnerValdiObject() const;
};

} // namespace Valdi
