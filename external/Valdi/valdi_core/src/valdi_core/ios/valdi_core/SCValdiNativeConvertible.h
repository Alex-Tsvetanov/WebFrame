#import <Foundation/Foundation.h>

@protocol SCValdiNativeConvertible <NSObject>

- (void)valdi_toNative:(void*)value;

@end
