#import "valdi_core/SCValdiINavigatorManagedViewController.h"
#import "valdi_core/SCValdiRootView.h"
#import <UIKit/UIKit.h>

@protocol SCValdiContainerViewController <SCValdiINavigatorManagedViewController>

- (instancetype)initWithValdiView:(SCValdiRootView*)valdiView;

/// This will be triggered from the TypeScript side when the component wants to prevent its container view controller
/// from being interactively dismissed.
- (void)forceDisableDismissalGesture:(BOOL)forceDisable;

@optional
// Only used for internal purposes to display the platform navigation bar.
// Do not rely on this for production applications.
- (void)setShowsNavigationBar:(BOOL)showsNavigationBar;

@end
