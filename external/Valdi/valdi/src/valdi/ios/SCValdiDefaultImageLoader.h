//
//  SCValdiDefaultImageLoader.h
//  ios
//
//  Created by Simon Corsin on 5/7/20.
//

#import "valdi_core/SCValdiImageLoader.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 A default, inefficient image loader. Not meant to use for production.
 */
@interface SCValdiDefaultImageLoader : NSObject <SCValdiImageLoader>

@end

NS_ASSUME_NONNULL_END
