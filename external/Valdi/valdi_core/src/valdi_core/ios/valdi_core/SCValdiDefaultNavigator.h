#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiContainerViewController.h"
#import "valdi_core/SCValdiINavigator.h"
#import "valdi_core/SCValdiINavigatorManagedViewController.h"

@protocol SCValdiRuntimeProtocol;
@protocol SCValdiContextProtocol;

NS_ASSUME_NONNULL_BEGIN

/// Default implementation of SCValdiINavigator, leverages UIKit's UIViewController presentation and dismissal for
/// `present` and `dismiss`, as well as the presence of an implicit UINavigationController for `push` and `pop`.
///
/// All you need to is instantiate SCValdiDefaultNavigator and set the `managedViewController` property to the view
/// controller that should be the one performing the present/dismiss/navigationController.push/navigationController.pop
/// calls.
@interface SCValdiDefaultNavigator : NSObject <SCValdiINavigator>

- (instancetype)initWithRuntime:(id<SCValdiRuntimeProtocol>)runtime;

/// The view controller that will be used to initiate the push/pop/present/dismiss operations.
///
/// The navigator object maintains a weak reference to the managed view controller to avoid retain cycles. It's common
/// for the managed view controller to own the view, whose context holds a reference to the navigator object itself.
@property (weak, nonatomic) UIViewController<SCValdiINavigatorManagedViewController>* managedViewController;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ////////////////////////////////////////////////////////////////////////////////////
// //                                                                                //
// //   The following properties and methods exist to provide customization points   //
// //   while using the default behavior provided by this class                      //
// //                                                                                //
// ////////////////////////////////////////////////////////////////////////////////////

/// Set this property if you'd like to wrap the presented/pushed page in a custom implementation of
/// SCValdiContainerViewController. By default SCValdiDefaultNavigator will instantate
/// SCValdiDefaultContainerViewController.
///
/// Note: Child pages will also be configured with the same override.
@property (strong, nonatomic, nullable) Class<SCValdiContainerViewController> containerClassOverride;

/// Set this property if you'd like to use a specific subclass of UINavigationController when presenting pages
/// with wrapInPlatformNavigationController set to true.
/// This may be useful if you'd like to, say, use a UINavigationController subclass that proves SCPageLogging
/// capability.
///
/// MUST BE A UINavigationController SUBCLASS
///
/// Note: Child pages will also be configured with the same override.
@property (strong, nonatomic, nullable) Class navigationControllerClassOverride;

/// Override this if you'd like to wrap the presented/pushed page in a completely custom view controller.
/// You should use this only if using containerClassOverride alone doesn't satisfy your needs.
///
/// Make sure you configure and provide a SCValdiINavigator instance to the wrapped page too.
- (UIViewController<SCValdiINavigatorManagedViewController>* _Nonnull)
    makeContainerViewControllerWithPage:(SCValdiINavigatorPageConfig* _Nonnull)page
                     parentValdiContext:(id<SCValdiContextProtocol> _Nullable)parentValdiContext;

@end

NS_ASSUME_NONNULL_END
