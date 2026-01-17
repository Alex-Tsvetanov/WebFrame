//
//  RuntimeWrapper.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//
#include "valdi/android/RuntimeWrapper.hpp"

namespace ValdiAndroid {

RuntimeWrapper::RuntimeWrapper(const Valdi::SharedRuntime& runtime,
                               RuntimeManagerWrapper* runtimeManagerWrapper,
                               float pointScale)
    : _runtime(runtime), _runtimeManagerWrapper(runtimeManagerWrapper), _pointScale(pointScale) {}

RuntimeWrapper::~RuntimeWrapper() {
    _runtime->fullTeardown();
}

Valdi::Runtime& RuntimeWrapper::getRuntime() const {
    return *_runtime;
}

const Valdi::SharedRuntime& RuntimeWrapper::getSharedRuntime() const {
    return _runtime;
}

RuntimeManagerWrapper* RuntimeWrapper::getRuntimeManagerWrapper() const {
    return _runtimeManagerWrapper;
}

float RuntimeWrapper::getPointScale() const {
    return _pointScale;
}

} // namespace ValdiAndroid
