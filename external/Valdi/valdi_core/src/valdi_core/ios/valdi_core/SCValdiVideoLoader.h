
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiBaseAssetLoader.h"
#import "valdi_core/SCValdiCancelable.h"
#import "valdi_core/SCValdiVideoPlayer.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^SCValdiVideoLoaderCompletion)(id<SCValdiVideoPlayer> _Nullable player, NSError* _Nullable error);

@protocol SCValdiVideoLoader <SCValdiBaseAssetLoader>

/**
 Load the video with the given request payload and call the completion when done.
 This method should only be implemented if this SCValdiVideoLoader instance supports loading
 URLs into SCValdiVideoPlayer instances.
 */
- (id<SCValdiCancelable>)loadVideoWithRequestPayload:(id)requestPayload
                                          parameters:(SCValdiAssetRequestParameters)parameters
                                          completion:(SCValdiVideoLoaderCompletion)completion;

@end

NS_ASSUME_NONNULL_END
