//
//  JavaUntypedClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 3/7/23.
//

#pragma once

#include "valdi/android/JavaClassDelegate.hpp"

namespace ValdiAndroid {

class IndirectJavaGlobalRef;

/**
 Concrete implementation of JavaClassDelegate for objects that should be marshalled as untyped.
 */
class JavaUntypedClassDelegate : public Valdi::SimpleRefCountable, public JavaClassDelegate {
public:
    JavaUntypedClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier, jclass cls);
    ~JavaUntypedClassDelegate() override;
};

} // namespace ValdiAndroid
