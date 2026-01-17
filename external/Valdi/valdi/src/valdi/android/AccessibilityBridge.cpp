#include "valdi/android/AccessibilityBridge.hpp"
#include "valdi/runtime/Views/ViewFactory.hpp"
#include "valdi/snap_drawing/Utils/ValdiUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

void AccessibilityBridge::walkViewNode(Valdi::ViewNode* viewNode, std::vector<Valdi::Value>& values, float pointScale) {
    // If the view node is an accessibility leaf, its children must not be fetched
    if (viewNode->isAccessibilityLeaf()) {
        AccessibilityBridge::writeInt32(0, values);
        return;
    }
    // Otherwise, treat the current node as a container, and recurse
    auto childrenViewNode = viewNode->getAccessibilityChildrenRecursive();
    AccessibilityBridge::writeInt32(static_cast<int32_t>(childrenViewNode.size()), values);
    for (Valdi::ViewNode* childViewNode : childrenViewNode) {
        AccessibilityBridge::writeViewNode(childViewNode, values, pointScale);
        AccessibilityBridge::walkViewNode(childViewNode, values, pointScale);
    }
}

void AccessibilityBridge::writeInt64(int64_t value, std::vector<Valdi::Value>& values) {
    values.emplace_back(value);
}

void AccessibilityBridge::writeInt32(int32_t value, std::vector<Valdi::Value>& values) {
    values.emplace_back(value);
}

void AccessibilityBridge::writeBool(bool value, std::vector<Valdi::Value>& values) {
    values.emplace_back(value);
}

void AccessibilityBridge::writeString(const Valdi::StringBox& value, std::vector<Valdi::Value>& values) {
    values.emplace_back(value);
}

void AccessibilityBridge::writeViewNode(Valdi::ViewNode* viewNode,
                                        std::vector<Valdi::Value>& values,
                                        float pointScale) {
    if (viewNode->hasView() && viewNode->getViewFactory()->isUserSpecified()) {
        AccessibilityBridge::writeBool(true, values);

        auto view = viewNode->getView();
        auto bridgeView = snap::drawing::bridgeViewFromView(view);

        if (bridgeView != nullptr) {
            values.emplace_back(bridgeView);
        } else {
            values.emplace_back(view);
        }
    } else {
        AccessibilityBridge::writeBool(false, values);
    }
    AccessibilityBridge::writeInt64(ValdiAndroid::bridgeRetain(viewNode), values);
    AccessibilityBridge::writeInt32(viewNode->getRawId(), values);

    AccessibilityBridge::writeInt32(viewNode->getAccessibilityCategory(), values);

    AccessibilityBridge::writeString(viewNode->getAccessibilityLabel(), values);
    AccessibilityBridge::writeString(viewNode->getAccessibilityHint(), values);
    AccessibilityBridge::writeString(viewNode->getAccessibilityValue(), values);
    AccessibilityBridge::writeString(viewNode->getAccessibilityId(), values);

    AccessibilityBridge::writeBool(viewNode->getAccessibilityStateDisabled(), values);
    AccessibilityBridge::writeBool(viewNode->getAccessibilityStateSelected(), values);
    AccessibilityBridge::writeBool(viewNode->getAccessibilityStateLiveRegion(), values);

    auto frameInRoot = viewNode->computeVisualFrameInRoot();
    AccessibilityBridge::writeInt32(frameInRoot.x * pointScale, values);
    AccessibilityBridge::writeInt32(frameInRoot.y * pointScale, values);
    AccessibilityBridge::writeInt32(frameInRoot.width * pointScale, values);
    AccessibilityBridge::writeInt32(frameInRoot.height * pointScale, values);
}

} // namespace ValdiAndroid
