//
//  VIewNodesFrameObserver.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/22/20.
//

#include "valdi/runtime/Context/ViewNodesFrameObserver.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"

namespace Valdi {

struct FrameChangeEvent {
    double id;
    double x;
    double y;
    double width;
    double height;
};

ViewNodesFrameObserver::ViewNodesFrameObserver() = default;

ViewNodesFrameObserver::~ViewNodesFrameObserver() = default;

void ViewNodesFrameObserver::flush() {
    if (_values == nullptr) {
        flushCompleteCallbacks();
        return;
    }
    auto values = std::move(_values);

    if (_callback != nullptr) {
        auto param = Value(makeShared<ValueTypedArray>(TypedArrayType::Float64Array, values->toBytesView()));
        (*_callback)({std::move(param)});
    }
    flushCompleteCallbacks();
}

void ViewNodesFrameObserver::flushCompleteCallbacks() {
    auto callbacks = std::move(_layoutCompletedCallbacks);
    for (const auto& completeCallback : callbacks) {
        (*completeCallback)();
    }
}

void ViewNodesFrameObserver::onFrameChanged(RawViewNodeId id, const Frame& frame) {
    if (_values == nullptr) {
        _values = makeShared<ByteBuffer>();
    }

    FrameChangeEvent event = {
        .id = static_cast<double>(id),
        .x = static_cast<double>(frame.x),
        .y = static_cast<double>(frame.y),
        .width = static_cast<double>(frame.width),
        .height = static_cast<double>(frame.height),
    };

    _values->append(reinterpret_cast<const Byte*>(&event), reinterpret_cast<const Byte*>(&event) + sizeof(event));
}

void ViewNodesFrameObserver::appendCompleteCallback(const Ref<ValueFunction>& callback) {
    _layoutCompletedCallbacks.emplace_back(callback);
}

void ViewNodesFrameObserver::setCallback(const Ref<ValueFunction>& callback) {
    _callback = callback;
}

} // namespace Valdi
