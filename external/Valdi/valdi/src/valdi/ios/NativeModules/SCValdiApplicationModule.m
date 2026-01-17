//
//  SCValdiApplicationModule.m
//  Valdi
//
//  Created by Simon Corsin on 9/19/18.
//

#import "valdi/ios/NativeModules/SCValdiApplicationModule.h"
#import "valdi/ios/NativeModules/SCValdiBridgeModuleUtils.h"

#import <UIKit/UIKit.h>

@implementation SCValdiApplicationModule {
    SCValdiBridgeObserver *_onBackgroundObserver;
    SCValdiBridgeObserver *_onForegroundObserver;
    SCValdiBridgeObserver *_onKeyboardObserver;

    dispatch_group_t _applicationModuleReadyGroup;
    BOOL _isApplicationModuleReady;
    NSString *_appVersion;

    // Mutable fields
    BOOL _isAppInForeground;
    BOOL _isIntegrationTestEnv;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        _onBackgroundObserver = [SCValdiBridgeObserver new];
        _onForegroundObserver = [SCValdiBridgeObserver new];
        _onKeyboardObserver = [SCValdiBridgeObserver new];

        NSNotificationCenter *notifications = NSNotificationCenter.defaultCenter;

        [notifications addObserver:self
                          selector:@selector(_didEnterBackground)
                              name:UIApplicationDidEnterBackgroundNotification
                            object:nil];
        [notifications addObserver:self
                          selector:@selector(_didBecomeActive)
                              name:UIApplicationDidBecomeActiveNotification
                            object:nil];
        [notifications addObserver:self
                          selector:@selector(_willResignActive)
                              name:UIApplicationWillResignActiveNotification
                            object:nil];
        [notifications addObserver:self
                          selector:@selector(_keyboardWillShow:)
                              name:UIKeyboardWillShowNotification
                            object:nil];
        [notifications addObserver:self
                          selector:@selector(_keyboardWillHide:)
                              name:UIKeyboardWillHideNotification
                            object:nil];

        _applicationModuleReadyGroup = dispatch_group_create();
        dispatch_group_enter(_applicationModuleReadyGroup);

        if (NSThread.isMainThread) {
            [self _initializeApplicationModuleIfNeeded];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{
                [self _initializeApplicationModuleIfNeeded];
            });
        }

        _appVersion = @"";
        id bundleVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:(NSString *)kCFBundleVersionKey];
        if ([bundleVersion isKindOfClass:[NSString class]]) {
            _appVersion = (NSString *)bundleVersion;
        }
    }
    return self;
}

- (void)ensureApplicationModuleIsReadyForContextCreation
{
    // The module is either already ready (init called on main thread) or _initializeApplicationModuleIfNeeded is
    // scheduled (init called on background thread).
    // If we're on the main thread here, we synchronously call _initializeApplicationModuleIfNeeded so we don't need
    // to wait on the scheduled call. This will prevent deadlocks if the callsite waits for the JS thread.
    // If we're not on the main thread, we don't need to do anything as the module is either ready or
    // _initializeApplicationModuleIfNeeded is already scheduled.
    if (NSThread.isMainThread) {
        [self _initializeApplicationModuleIfNeeded];
    }
}

- (void)_initializeApplicationModuleIfNeeded
{
    // Run on main thread
    if (_isApplicationModuleReady) {
        return;
    }

    BOOL isAppInForeground = [UIApplication sharedApplication].applicationState == UIApplicationStateActive;
    [self _setIsAppInForeground:isAppInForeground];

    _isApplicationModuleReady = YES;
    dispatch_group_leave(_applicationModuleReadyGroup);
}

- (void)_didEnterBackground
{
    [_onBackgroundObserver notifyWithMarshaller:nil];
}

- (void)_didBecomeActive
{
    [self _setIsAppInForeground:YES];
    [_onForegroundObserver notifyWithMarshaller:nil];
}

- (void)_willResignActive
{
    [self _setIsAppInForeground:NO];
}

- (void)_keyboardWillShow:(NSNotification *)notification
{
    CGRect keyboardFrame = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGFloat keyboardHeightInFrame = keyboardFrame.size.height;

    SCValdiMarshallerScoped(marshaller, {
        SCValdiMarshallerPushDouble(marshaller, keyboardHeightInFrame);
        [_onKeyboardObserver notifyWithMarshaller:marshaller];
    });
}

- (void)_keyboardWillHide:(NSNotification *)notification
{
    SCValdiMarshallerScoped(marshaller, {
        SCValdiMarshallerPushDouble(marshaller, 0.0);
        [_onKeyboardObserver notifyWithMarshaller:marshaller];
    });
}

- (void)_setIsAppInForeground:(BOOL)isAppInForeground
{
    @synchronized(self) {
        _isAppInForeground = isAppInForeground;
    }
}

- (void)setIsIntegrationTestEnvironment:(BOOL)isIntegrationTestEnvironment
{
    @synchronized(self) {
        _isIntegrationTestEnv = isIntegrationTestEnvironment;
    }
}

- (void)_isForegrounded:(SCValdiMarshaller *)marshaller
{
    // Valdi calls this method from JS thread
    @synchronized(self) {
        SCValdiMarshallerPushBool(marshaller, _isAppInForeground);
    }
}

- (void)_isIntegrationTestEnvironment:(SCValdiMarshaller *)marshaller
{
    // Valdi calls this method from JS thread
    @synchronized(self) {
        SCValdiMarshallerPushBool(marshaller, _isIntegrationTestEnv);
    }
}

- (void)_getAppVersion:(SCValdiMarshaller *)marshaller
{
    // Valdi calls this method from JS thread
    SCValdiMarshallerPushString(marshaller, _appVersion);
}

- (NSString *)getModulePath
{
    return @"valdi_core/src/ApplicationBridge";
}

- (NSObject *)loadModule
{
    // Synchronously wait until this module has been initialized so that we don't provide incorrect values
    // to TypeScript
    dispatch_group_wait(_applicationModuleReadyGroup, DISPATCH_TIME_FOREVER);

    return @{
        @"observeEnteredBackground" : _onBackgroundObserver,
        @"observeEnteredForeground" : _onForegroundObserver,
        @"observeKeyboardHeight" : _onKeyboardObserver,
        @"isForegrounded" : BRIDGE_METHOD(_isForegrounded),
        @"isIntegrationTestEnvironment" : BRIDGE_METHOD(_isIntegrationTestEnvironment),
        @"getAppVersion" : BRIDGE_METHOD(_getAppVersion),
    };
}

@end
