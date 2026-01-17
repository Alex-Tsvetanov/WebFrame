#pragma once

#include "valdi_core/JavaScriptEngineType.hpp"

namespace Valdi {

class IJavaScriptBridge;

class JavaScriptBridge {
public:
    // Modes of Operation:
    //  macOS/linux:
    //     auto   : QuickJS
    //     jscore : enabled
    //     QuickJS: enabled
    // iOS:
    //     auto   : JSCore
    //     jscore : enabled
    //     QuickJS: enabled for client_development/production_development / Disabled otherwise
    // android:
    //     auto  : JSCore client_development/production_development / QuickJS otherwise
    //     jscore: enabled for client_development/production_development / Disabled otherwise
    //     QuickJS: enabled
    static IJavaScriptBridge* get(
        snap::valdi_core::JavaScriptEngineType type = snap::valdi_core::JavaScriptEngineType::Auto);
};

}; // namespace Valdi
