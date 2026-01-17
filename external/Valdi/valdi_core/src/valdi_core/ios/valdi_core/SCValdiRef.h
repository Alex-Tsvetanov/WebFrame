//
//  SCValdiRef.h
//  valdi-ios
//
//  Created by Simon Corsin on 12/21/18.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiRef : NSObject

@property (strong, nonatomic) id instance;

- (instancetype)init;
- (instancetype)initWithInstance:(id)instance strong:(BOOL)strong;

- (void)makeStrong;
- (void)makeWeak;

@end

NS_ASSUME_NONNULL_END
