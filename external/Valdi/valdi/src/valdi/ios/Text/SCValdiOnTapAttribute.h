//
//  SCValdiOnTapAttribute.h
//  valdi-ios
//
//  Created by Simon Corsin on 12/21/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiFunction;

/**
 SCValdiOnTapAttribute holds an onTapp callback.
 Used to implement onTap within an SCValdiAttributedText.
 */
@interface SCValdiOnTapAttribute : NSObject

@property (readonly, strong, nonatomic) id<SCValdiFunction> callback;

- (instancetype)initWithCallback:(id<SCValdiFunction>)callback;

@end

NS_ASSUME_NONNULL_END
