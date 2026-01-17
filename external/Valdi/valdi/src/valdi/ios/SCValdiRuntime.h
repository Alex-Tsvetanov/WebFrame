//
//  SCValdiViewNodeLoader.h
//  Valdi
//
//  Created by Simon Corsin on 12/11/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import "valdi_core/SCNValdiCoreModuleFactory.h"
#import "valdi_core/SCValdiJSQueueDispatcher.h"
#import "valdi_core/SCValdiRuntimeProtocol.h"

#import <Foundation/Foundation.h>

@class UIView;
@class SCValdiDocument;
@class SCValdiView;
@class SCValdiRemoteBundle;
@protocol SCValdiJSRuntime;
@class SCValdiDeviceModule;
@class SCValdiContext;

typedef void (*SCValdiLoggerOutputFunction)(NSString* str);

@interface SCValdiRuntime : NSObject <SCValdiJSQueueDispatcher, SCValdiRuntimeProtocol>

@property (assign, nonatomic) BOOL isBackedByRemoteFiles;

@property (assign, nonatomic) BOOL disableLegacyMeasureBehaviorByDefault;

@property (readonly, nonatomic) SCValdiDeviceModule* deviceModule;

/**
 Asynchronously load a module, its dependencies and resources, and call the completion block when the load completes.
 If the module is stored remotely, this will fetch the module and store it in the disk cache.
 The completion is called on arbitrary thread.
 */
- (void)loadModule:(NSString*)moduleName completion:(void (^)(NSError* error))completion;

- (void)setPerformHapticFeedbackFunctionBlock:(void (^)(NSString*))block;

- (NSArray<SCValdiContext*>*)getAllContexts;

- (void)setAllowDarkMode:(BOOL)allowDarkMode
    useScreenUserInterfaceStyleForDarkMode:(BOOL)useScreenUserInterfaceStyleForDarkMode;

- (void)setIsIntegrationTestEnvironment:(BOOL)isIntegrationTestEnvironment;

- (void)applicationWillTerminate;

- (void)emitInitMetrics;

// Returns a dictionary of all loaded modules with their hashes
- (NSDictionary<NSString*, NSString*>*)getAllModuleHashes;

@end
