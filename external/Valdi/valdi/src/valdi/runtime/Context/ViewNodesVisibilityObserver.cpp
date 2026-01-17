//
//  ViewNodesVisibilityObserver.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/6/19.
//

#include "valdi/runtime/Context/ViewNodesVisibilityObserver.hpp"
#include "valdi/runtime/Context/Context.hpp"
#include "valdi/runtime/Utils/MainThreadManager.hpp"
#include "valdi_core/cpp/Utils/TimePoint.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"

namespace Valdi {

ViewNodesVisibilityObserver::ViewNodesVisibilityObserver(MainThreadManager* mainThreadManager)
    : _mainThreadManager(mainThreadManager) {}

ViewNodesVisibilityObserver::~ViewNodesVisibilityObserver() = default;

void ViewNodesVisibilityObserver::flush(const Ref<Context>& context) {
    if (_isFlushing || (_visibilityUpdates.empty() && _viewportUpdates.empty()) || _callback == nullptr) {
        return;
    }

    auto eventTime = _lastUpdateTime;
    _lastUpdateTime = TimePoint();

    size_t visibleIdsSize = 0;
    size_t invisibleIdsSize = 0;

    for (const auto& it : _visibilityUpdates) {
        if (it.second) {
            visibleIdsSize++;
        } else {
            invisibleIdsSize++;
        }
    }

    auto visibleIds = ValueArray::make(visibleIdsSize);
    auto invisibleIds = ValueArray::make(invisibleIdsSize);
    size_t visibleIdsIndex = 0;
    size_t invisibleIdsIndex = 0;

    for (const auto& it : _visibilityUpdates) {
        if (it.second) {
            (*visibleIds)[visibleIdsIndex++] = Value(it.first);
        } else {
            (*invisibleIds)[invisibleIdsIndex++] = Value(it.first);
        }
    }

    auto viewportUpdates = ValueArray::make(_viewportUpdates.size() * 5);

    size_t index = 0;
    for (const auto& it : _viewportUpdates) {
        auto frame = it.second;
        viewportUpdates->emplace(index++, Value(it.first));
        viewportUpdates->emplace(index++, Value(frame.x));
        viewportUpdates->emplace(index++, Value(frame.y));
        viewportUpdates->emplace(index++, Value(frame.width));
        viewportUpdates->emplace(index++, Value(frame.height));
    }

    _visibilityUpdates.clear();
    _viewportUpdates.clear();
    _isFlushing = true;

    auto updateId = context->enqueueUpdate();

    auto self = strongRef(this);
    auto callback = makeShared<ValueFunctionWithCallable>(
        [self, context, updateId](const ValueFunctionCallContext& /*callContext*/) -> Value {
            DispatchFunction dispatchFn = [self, context, updateId]() {
                self->onFlushAcknowledged(context);
                context->markUpdateCompleted(updateId);
            };
            if (self->_mainThreadManager != nullptr) {
                self->_mainThreadManager->dispatch(context, std::move(dispatchFn));
            } else {
                dispatchFn();
            }

            return Value::undefined();
        });

    (*_callback)(
        {Value(visibleIds), Value(invisibleIds), Value(viewportUpdates), Value(callback), Value(eventTime.getTime())});
}

void ViewNodesVisibilityObserver::onFlushAcknowledged(const Ref<Context>& context) {
    _isFlushing = false;
    // Flush again in case we had a new change since then
    flush(context);
}

void ViewNodesVisibilityObserver::onViewNodeVisibilityChanged(RawViewNodeId id, bool visible) {
    auto it = _visibilityUpdates.find(id);

    if (it == _visibilityUpdates.end()) {
        // First time we update the visibility of this node
        _visibilityUpdates[id] = visible;
    } else {
        if (it->second != visible) {
            // If we had stored a change and that we are reversing back, we
            // can just remove the pending visibility update.
            _visibilityUpdates.erase(it);
        }
    }

    updateLastUpdateTimeIfNeeded();
}

void ViewNodesVisibilityObserver::onViewNodeViewportChanged(RawViewNodeId id, const Frame& viewport) {
    _viewportUpdates[id] = viewport;

    updateLastUpdateTimeIfNeeded();
}

void ViewNodesVisibilityObserver::updateLastUpdateTimeIfNeeded() {
    if (_lastUpdateTime.isEmpty()) {
        _lastUpdateTime = TimePoint::now();
    }
}

void ViewNodesVisibilityObserver::setCallback(const Ref<ValueFunction>& callback) {
    _callback = callback;
}

} // namespace Valdi
