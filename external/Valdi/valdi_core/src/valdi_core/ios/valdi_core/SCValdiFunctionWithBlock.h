//
//  SCValdiFunctionWithBlock.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/19/19.
//

#import "valdi_core/SCValdiActionCompat.h"
#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiMarshaller.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiFunctionWithBlock : SCValdiActionCompat <SCValdiFunction>

+ (instancetype)functionWithBlock:(SCValdiFunctionBlock)block;

@end

NS_ASSUME_NONNULL_END
