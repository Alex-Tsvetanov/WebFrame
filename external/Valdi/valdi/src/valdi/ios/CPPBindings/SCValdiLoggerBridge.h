//
//  SCValdiLogger.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#ifdef __cplusplus

#import "valdi/ios/SCValdiRuntime.h"

#import "valdi_core/cpp/Interfaces/ILogger.hpp"

#import <Foundation/Foundation.h>

namespace ValdiIOS {

class Logger : public Valdi::ILogger {
public:
    Logger();

    bool isEnabledForType(Valdi::LogType type) override;

    void log(Valdi::LogType type, std::string messsage) override;
};
} // namespace ValdiIOS

#endif
