//
//  JavaScriptBridgedObjectLeakTracker.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/17/19.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {
class ILogger;

class JavaScriptBridgedObjectLeakTracker : public ValdiObject {
public:
    JavaScriptBridgedObjectLeakTracker(ILogger& logger, StringBox name);

    ~JavaScriptBridgedObjectLeakTracker() override;

    VALDI_CLASS_HEADER(JavaScriptBridgedObjectLeakTracker)

private:
    [[maybe_unused]] ILogger& _logger;
    StringBox _name;
};
} // namespace Valdi
