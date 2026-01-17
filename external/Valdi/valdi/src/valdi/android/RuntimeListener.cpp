//
//  ContextManager.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/13/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/android/RuntimeListener.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/jni/JavaCache.hpp"

namespace ValdiAndroid {

RuntimeListener::RuntimeListener(JavaEnv env, jobject contextManager)
    : _javaRef(env, contextManager, "ContextManager") {
    auto object = _javaRef.toObject();
    auto cls = object.getClass();
    cls.getMethod("createContext", _createContextMethod);
    cls.getMethod("destroyContext", _destroyContextMethod);

    cls.getMethod("onContextRendered", _onContextRendered);
    cls.getMethod("onContextLayoutBecameDirty", _onContextLayoutBecameDirty);
    cls.getMethod("onAllContextsDestroyed", _onAllContextsDestroyed);
}
RuntimeListener::~RuntimeListener() = default;

void RuntimeListener::onContextCreated(Valdi::Runtime& /*runtime*/, const Valdi::SharedContext& context) {
    auto wrappedContext = bridgeRetain(context.get());

    auto javaRef = _javaRef.toObject();
    auto attachedObject = _attachedObject.toObject();

    auto createdContext = _createContextMethod.call(javaRef,
                                                    static_cast<int64_t>(wrappedContext),
                                                    static_cast<int32_t>(context->getContextId()),
                                                    toJavaObject(JavaEnv(), context->getPath().toString()),
                                                    toJavaObject(JavaEnv(), context->getAttribution().moduleName),
                                                    toJavaObject(JavaEnv(), context->getAttribution().owner),
                                                    attachedObject);

    auto userData = toValdiContextUserData(createdContext);
    context->setUserData(userData);
}

void RuntimeListener::onContextDestroyed(Valdi::Runtime& /*runtime*/, Valdi::Context& context) {
    auto valdiContext = fromValdiContextUserData(context.getUserData());
    if (!valdiContext.isNull()) {
        _destroyContextMethod.call(_javaRef.toObject(), valdiContext);
    }

    context.setUserData(nullptr);
}

void RuntimeListener::onContextRendered(Valdi::Runtime& /*runtime*/, const Valdi::SharedContext& context) {
    auto valdiContext = fromValdiContextUserData(context->getUserData());

    _onContextRendered.call(_javaRef.toObject(), valdiContext);
}

void RuntimeListener::onContextLayoutBecameDirty(Valdi::Runtime& /*runtime*/, const Valdi::SharedContext& context) {
    auto valdiContext = fromValdiContextUserData(context->getUserData());

    _onContextLayoutBecameDirty.call(_javaRef.toObject(), valdiContext);
}

void RuntimeListener::onAllContextsDestroyed(Valdi::Runtime& /*runtime*/) {
    _onAllContextsDestroyed.call(_javaRef.toObject(), _attachedObject.toObject());
}

void RuntimeListener::setAttachedObject(GlobalRefJavaObjectBase attachedObject) {
    _attachedObject = std::move(attachedObject);
}

const GlobalRefJavaObjectBase& RuntimeListener::getAttachedObject() const {
    return _attachedObject;
}

} // namespace ValdiAndroid
