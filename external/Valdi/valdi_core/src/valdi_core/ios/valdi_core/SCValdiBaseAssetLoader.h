
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef struct SCValdiAssetRequestParameters {
    NSInteger preferredWidth;
    NSInteger preferredHeight;
} SCValdiAssetRequestParameters;

@protocol SCValdiBaseAssetLoader <NSObject>

/**
 Returns the URL schemes that this VideoLoader knows how to load.
 */
- (NSArray<NSString*>*)supportedURLSchemes;

/**
 Returns the request payload object which will be provided to the loadVideoWithRequestPayload:
 method for the given imageURL. The payload can be any Objective-C object. The payload might
 be cached internally by the framework.
 */
- (id)requestPayloadWithURL:(NSURL*)imageURL error:(NSError**)error;

@end

NS_ASSUME_NONNULL_END
