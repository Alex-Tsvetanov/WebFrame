//
//  JavaScriptCapturedStacktrace.hpp
//  valdi
//
//  Created by Simon Corsin on 07/30/2024.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class StringBox;
class Context;

class JavaScriptCapturedStacktrace {
public:
    enum class Status {
        NOT_RUNNING = 0,
        RUNNING,
        TIMED_OUT,
    };

    JavaScriptCapturedStacktrace(Status status, const StringBox& stackTrace, const Ref<Context>& context);
    ~JavaScriptCapturedStacktrace();

    constexpr Status getStatus() const {
        return _status;
    }

    constexpr const StringBox& getStackTrace() const {
        return _stackTrace;
    }

    const Ref<Context>& getContext() const;

private:
    Status _status;
    StringBox _stackTrace;
    Ref<Context> _context;
};

} // namespace Valdi
