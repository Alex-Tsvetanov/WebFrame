
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiBaseAssetLoader.h"
#import "valdi_core/SCValdiCancelable.h"
#import "valdi_core/SCValdiImage.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^SCValdiImageLoaderCompletion)(SCValdiImage* _Nullable image, NSError* _Nullable error);
typedef void (^SCValdiImageLoaderBytesCompletion)(NSData* _Nullable data, NSError* _Nullable error);

@protocol SCValdiImageLoader <SCValdiBaseAssetLoader>

@optional

/**
 Load the image with the given request payload and call the completion when done.
 This method should only be implemented if this SCValdiImageLoader instance supports loading
 URLs into SCValdiImage instances.
 */
- (id<SCValdiCancelable>)loadImageWithRequestPayload:(id)requestPayload
                                          parameters:(SCValdiAssetRequestParameters)parameters
                                          completion:(SCValdiImageLoaderCompletion)completion;

/**
 Load the bytes with the given request payload and call the completion when done.
 This method should only be implemented if this SCValdiImageLoader instance supports loading
 URLs into raw bytes.
 */
- (id<SCValdiCancelable>)loadBytesWithRequestPayload:(id)requestPayload
                                          completion:(SCValdiImageLoaderBytesCompletion)completion;

@end

NS_ASSUME_NONNULL_END
