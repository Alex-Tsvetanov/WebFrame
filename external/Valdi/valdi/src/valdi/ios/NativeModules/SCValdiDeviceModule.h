//
//  SCValdiDeviceModule.h
//  Valdi
//
//  Created by Simon Corsin on 9/6/18.
//

#import "valdi_core/SCNValdiCoreModuleFactory.h"
#import "valdi_core/SCValdiExceptionReporter.h"
#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiJSQueueDispatcher.h"

#import <Foundation/Foundation.h>

@interface SCValdiDeviceModule : NSObject <SCNValdiCoreModuleFactory>

@property (atomic, strong) id<SCValdiFunction> performHapticFeedbackFunction;

@property (atomic, weak) id<SCValdiExceptionReporter> exceptionReporter;

- (instancetype)initWithJSQueueDispatcher:(id<SCValdiJSQueueDispatcher>)jsQueueDispatcher;
- (void)setAllowDarkMode:(BOOL)allowDarkMode
    useScreenUserInterfaceStyleForDarkMode:(BOOL)useScreenUserInterfaceStyleForDarkMode;
- (void)ensureDeviceModuleIsReadyForContextCreation;

@end
