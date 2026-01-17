//
//  SCValdiSharedLogger.m
//  valdi-ios
//
//  Created by Simon Corsin on 4/1/19.
//

#import "valdi_core/SCValdiSharedLogger.h"

static id<SCValdiLogger> _sharedLogger;

__nullable id<SCValdiLogger> SCValdiGetSharedLogger() {
    return _sharedLogger;
}

void SCValdiSetSharedLogger(id<SCValdiLogger> logger) {
    _sharedLogger = logger;
}
