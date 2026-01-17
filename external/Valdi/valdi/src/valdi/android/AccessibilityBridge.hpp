#pragma once

#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include <vector>

namespace ValdiAndroid {

class AccessibilityBridge {
public:
    static void walkViewNode(Valdi::ViewNode* viewNode, std::vector<Valdi::Value>& values, float pointScale);

private:
    static void writeInt64(int64_t value, std::vector<Valdi::Value>& values);
    static void writeInt32(int32_t value, std::vector<Valdi::Value>& values);
    static void writeBool(bool value, std::vector<Valdi::Value>& values);
    static void writeString(const Valdi::StringBox& value, std::vector<Valdi::Value>& values);
    static void writeViewNode(Valdi::ViewNode* viewNode, std::vector<Valdi::Value>& values, float pointScale);
};

} // namespace ValdiAndroid
