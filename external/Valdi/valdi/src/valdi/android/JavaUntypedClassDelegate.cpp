//
//  JavaUntypedClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 3/7/23.
//

#include "valdi/android/JavaUntypedClassDelegate.hpp"

namespace ValdiAndroid {

JavaUntypedClassDelegate::JavaUntypedClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier, jclass cls)
    : JavaClassDelegate(identifier, cls) {}

JavaUntypedClassDelegate::~JavaUntypedClassDelegate() = default;

} // namespace ValdiAndroid
