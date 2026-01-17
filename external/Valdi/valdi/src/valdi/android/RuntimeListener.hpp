//
//  RuntimeListener.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/13/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/IRuntimeListener.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace Valdi {
class Runtime;
}

namespace ValdiAndroid {

class RuntimeListener : public Valdi::IRuntimeListener {
public:
    RuntimeListener(JavaEnv env, jobject contextManager);
    ~RuntimeListener() override;

    void onContextCreated(Valdi::Runtime& runtime, const Valdi::SharedContext& context) override;
    void onContextDestroyed(Valdi::Runtime& runtime, Valdi::Context& context) override;

    void onContextRendered(Valdi::Runtime& runtime, const Valdi::SharedContext& context) override;

    void onContextLayoutBecameDirty(Valdi::Runtime& runtime, const Valdi::SharedContext& context) override;

    void onAllContextsDestroyed(Valdi::Runtime& runtime) override;

    void setAttachedObject(GlobalRefJavaObjectBase attachedObject);
    const GlobalRefJavaObjectBase& getAttachedObject() const;

private:
    GlobalRefJavaObjectBase _javaRef;
    JavaMethod<ValdiContext, int64_t, int32_t, String, String, String, JavaObject> _createContextMethod;
    JavaMethod<VoidType, ValdiContext> _destroyContextMethod;

    JavaMethod<VoidType, ValdiContext> _onContextRendered;
    JavaMethod<VoidType, ValdiContext> _onContextLayoutBecameDirty;
    JavaMethod<VoidType, JavaObject> _onAllContextsDestroyed;

    GlobalRefJavaObjectBase _attachedObject;
};

} // namespace ValdiAndroid
