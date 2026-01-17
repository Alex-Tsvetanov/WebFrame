//
//  ContextId.cpp
//  valdi-desktop-apple
//
//  Created by Ramzy Jaber on 8/16/22.
//

#include "valdi/runtime/Context/RenderingBackendType.hpp"

namespace Valdi {

const StringBox& getBackendString(RenderingBackendType type) {
    static auto kNativeBackend = STRING_LITERAL("native");
    static auto kSnapdrawingBackend = STRING_LITERAL("snap_drawing");
    static auto kUnset = STRING_LITERAL("unset");
    switch (type) {
        case RenderingBackendTypeNative:
            return kNativeBackend;
        case RenderingBackendTypeSnapDrawing:
            return kSnapdrawingBackend;
        case RenderingBackendTypeUnset:
            return kUnset;
    }
}

} // namespace Valdi
