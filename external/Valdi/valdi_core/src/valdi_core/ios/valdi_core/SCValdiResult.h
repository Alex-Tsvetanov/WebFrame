//
//  SCValdiResult.h
//  valdi-ios
//
//  Created by Simon Corsin on 9/5/19.
//

#import "valdi_core/SCMacros.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiResult : NSObject

@property (readonly, nonatomic) BOOL success;

@property (readonly, nonatomic) id successValue;
@property (readonly, nonatomic) NSString* errorMessage;

VALDI_NO_INIT

@end

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

SCValdiResult* SCValdiResultSuccess();
SCValdiResult* SCValdiResultSuccessWithData(id data);
SCValdiResult* SCValdiResultFailure(NSString* errorMessage);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

NS_ASSUME_NONNULL_END
