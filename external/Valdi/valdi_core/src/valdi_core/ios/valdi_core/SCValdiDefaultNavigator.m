#import "SCValdiDefaultNavigator.h"

#import "valdi_core/SCValdiRuntimeProtocol.h"
#import "valdi_core/SCValdiComponentContainerView.h"
#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/SCValdiINavigator.h"
#import "valdi_core/SCValdiDefaultContainerViewController.h"

@interface SCValdiDefaultNavigator ()

@end

@implementation SCValdiDefaultNavigator {
    __weak id<SCValdiRuntimeProtocol> _runtime;
}

- (instancetype)initWithRuntime:(id<SCValdiRuntimeProtocol>)runtime
{
    self = [super init];
    if (self) {
        _runtime = runtime;
    }
    return self;
}

- (UIViewController<SCValdiINavigatorManagedViewController> *)viewController {
    UIViewController<SCValdiINavigatorManagedViewController> *viewController = self.managedViewController;
    if (!viewController) {
        SCLogValdiError(@"SCValdiDefaultNavigator attempting an operation, but managedViewController is nil.");
    }
    return viewController;
}

- (void)dismissWithAnimated:(BOOL)animated {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.viewController dismissViewControllerAnimated:animated completion:nil];
    });
}

- (void)popToSelfWithAnimated:(BOOL)animated {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.viewController.navigationController popToViewController:self.viewController animated:animated];
    });
}

- (void)popWithAnimated:(BOOL)animated {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.viewController.navigationController popViewControllerAnimated:animated];
    });
}

- (void)presentComponentWithPage:(SCValdiINavigatorPageConfig * _Nonnull)page animated:(BOOL)animated {
    id<SCValdiContextProtocol> context = [_runtime currentContext];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self _doPresentComponentWithPage:page sourceComponentContext:context animated:animated];
    });
}

- (void)pushComponentWithPage:(SCValdiINavigatorPageConfig * _Nonnull)page animated:(BOOL)animated {
    id<SCValdiContextProtocol> context = [_runtime currentContext];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self _doPushComponentWithPage:page sourceComponentContext:context animated:animated];
    });
}

- (void)setBackButtonObserverWithObserver:(SCValdiINavigatorSetBackButtonObserverObserverBlock _Nonnull)observer {
    // Not implemented on iOS
}


- (void)setPageVisibilityObserverWithObserver:(SCValdiINavigatorSetPageVisibilityObserverObserverBlock)observer {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.viewController respondsToSelector:@selector(setPageVisibilityObserver:)]) {
            [self.viewController setPageVisibilityObserver:observer];
        } else {
            SCLogValdiError(@"SCValdiDefaultNavigator attempting to call setPageVisibilityObserver, but managedViewController doesn't implement it: %@", self.viewController);
        }
    });
}

- (void)forceDisableDismissalGestureWithForceDisable:(BOOL)forceDisable {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.viewController respondsToSelector:@selector(forceDisableDismissalGesture:)]) {
            [self.viewController forceDisableDismissalGesture:forceDisable];
        } else {
            SCLogValdiError(@"SCValdiDefaultNavigator attempting to call forceDisableDismissalGesture, but managedViewController doesn't implement it: %@", self.viewController);
        }
    });
}

- (UIViewController *)_findTopViewController
{
    UIViewController *topViewController = self.viewController;
    // Recursively find the top vc as long as its not being dismissed, since pushing or presenting from a dismissing vc is a no-op / invalid
    while (topViewController.presentedViewController && !topViewController.presentedViewController.beingDismissed) {
        topViewController = topViewController.presentedViewController;
    }
    return topViewController;
}

- (void)_doPresentComponentWithPage:(SCValdiINavigatorPageConfig * _Nonnull)page
             sourceComponentContext:(id<SCValdiContextProtocol> _Nullable)sourceComponentContext
                           animated:(BOOL)animated {
    UIViewController *viewController = [self makeContainerViewControllerWithPage:page parentValdiContext:sourceComponentContext];

    UIViewController *vcToPresentFrom = [self _findTopViewController];
    if ([page.wrapInPlatformNavigationController boolValue]) {
        UINavigationController *navigationController = [self wrapViewControllerInNavigationController:viewController];
        viewController = navigationController;
    }
    [vcToPresentFrom presentViewController:viewController animated:animated completion:nil];

}

- (void)_doPushComponentWithPage:(SCValdiINavigatorPageConfig * _Nonnull)page
          sourceComponentContext:(id<SCValdiContextProtocol> _Nullable)sourceComponentContext
                        animated:(BOOL)animated
{
    UIViewController *topViewController = [self _findTopViewController];
    UINavigationController *navigationController;
    if ([topViewController isKindOfClass:[UINavigationController class]]) {
        navigationController = (UINavigationController *)topViewController;
    } else {
        navigationController = topViewController.navigationController;
    }
    if (!navigationController) {
        SCLogValdiError(@"SCValdiDefaultNavigator failed to resolve a UINavigationController to push a new page on (page componentPath: %@)", page.componentPath);
        return;
    }
    UIViewController *viewController = [self makeContainerViewControllerWithPage:page parentValdiContext:sourceComponentContext];
    [navigationController pushViewController:viewController animated:animated];
}

- (UINavigationController *)wrapViewControllerInNavigationController:(UIViewController *)viewController {
    Class navigationControllerClassToInstantiate = [UINavigationController class];
    if (self.navigationControllerClassOverride) {
        navigationControllerClassToInstantiate = self.navigationControllerClassOverride;
    }
    UINavigationController *navigationController = (UINavigationController *)[[navigationControllerClassToInstantiate alloc] initWithRootViewController:viewController];
    navigationController.navigationBar.translucent = NO;
    return navigationController;

}

- (UIViewController<SCValdiContainerViewController> * _Nonnull)makeContainerViewControllerWithPage:(SCValdiINavigatorPageConfig * _Nonnull)page parentValdiContext:(id<SCValdiContextProtocol> _Nullable)parentValdiContext
{
    SCValdiDefaultNavigator *navigator = [[[self class] alloc] initWithRuntime:_runtime];
    navigator.containerClassOverride = self.containerClassOverride;
    navigator.navigationControllerClassOverride = self.navigationControllerClassOverride;

    NSMutableDictionary *contextWithNavigator = [page.componentContext mutableCopy];
    contextWithNavigator[@"navigator"] = navigator;
    SCValdiComponentContainerView *valdiView = [[SCValdiComponentContainerView alloc] initWithComponentPath:page.componentPath
                                                                                                               owner:nil
                                                                                                           viewModel:page.componentViewModel
                                                                                                    componentContext:[contextWithNavigator copy]
                                                                                                          runtime:_runtime];

    if (parentValdiContext) {
        [valdiView.valdiContext setParentContext:parentValdiContext];
    }
    Class classToInstantiate = [SCValdiDefaultContainerViewController class];
    if (self.containerClassOverride) {
        classToInstantiate = self.containerClassOverride;
    }
    UIViewController<SCValdiContainerViewController> *viewController = [[classToInstantiate alloc] initWithValdiView:valdiView];
    if ([viewController respondsToSelector:@selector(setShowsNavigationBar:)]) {
        [viewController setShowsNavigationBar:[page.showPlatformNavigationBar boolValue]];
    }
    if ([viewController respondsToSelector:@selector(setTitle:)]) {
        [viewController setTitle:page.platformNavigationTitle];
    }

    navigator.managedViewController = viewController;

    return viewController;
}

#pragma mark - Valdi marshalling

- (NSInteger)pushToValdiMarshaller:(nonnull SCValdiMarshallerRef)marshaller {
    return SCValdiINavigatorMarshall(marshaller, self);
}

- (BOOL)shouldRetainInstanceWhenMarshalling
{
    return YES;
}

@end
