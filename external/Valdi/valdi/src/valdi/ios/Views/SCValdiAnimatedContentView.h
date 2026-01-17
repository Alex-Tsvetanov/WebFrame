#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiViewAssetHandlingProtocol.h"
#import "valdi_core/SCValdiViewComponent.h"

/**
 * A view for playing back Lottie and other image animations.
 */
@interface SCValdiAnimatedContentView : SCValdiView <SCValdiViewComponent, SCValdiViewAssetHandlingProtocol>

@end
