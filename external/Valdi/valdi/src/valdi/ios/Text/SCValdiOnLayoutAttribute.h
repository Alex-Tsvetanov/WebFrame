
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiFunction;

/**
 SCValdiOnLayoutAttribute holds an onLayout callback.
 Used to implement onLayout within an SCValdiAttributedText.
 */
@interface SCValdiOnLayoutAttribute : NSObject

@property (readonly, strong, nonatomic) id<SCValdiFunction> callback;

- (instancetype)initWithCallback:(id<SCValdiFunction>)callback;

@end

NS_ASSUME_NONNULL_END
