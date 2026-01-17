#import "valdi_core/SCValdiINavigator.h"
#import "valdi_core/SCValdiRootViewProtocol.h"
#import "valdi_core/SCValdiRuntimeProtocol.h"
#import <UIKit/UIKit.h>

@interface SCValdiBootstrappingDependencies : NSObject

@property (strong, nonatomic) id<SCValdiRuntimeProtocol> runtime;
@property (strong, nonatomic) id<SCValdiINavigator> navigator;

@end

/**
 * A helper UIApplicationDelegate implementation that helps bootstrap a Valdi application.
 */
@interface SCValdiBootstrappingAppDelegate : UIResponder <UIApplicationDelegate>

/**
 * Returns the root component path to use as the root component within the application.
 */
- (NSString*)rootValdiComponentPath;

/**
 * Creates the root view to use for the application. If not overriden, will use a default
 * implementation that relies on "rootValdiComponentPath" to be overriden. This method
 * can be overriden instead if one needs more customization over how the view should be creataed.
 */
- (UIView<SCValdiRootViewProtocol>*)createRootViewWithDependencies:(SCValdiBootstrappingDependencies*)dependencies;

@end