#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiINavigator.h"

@protocol SCValdiINavigatorManagedViewController <NSObject>

@optional
/// This will be triggered from the TypeScript side when the component wants to prevent its container view controller
/// from being interactively dismissed.
- (void)forceDisableDismissalGesture:(BOOL)forceDisable;

- (void)setPageVisibilityObserver:(SCValdiINavigatorSetPageVisibilityObserverObserverBlock _Nullable)observer;

@end
