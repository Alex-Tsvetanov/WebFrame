//
//  SCValdiMacOSFunction.h
//  valdi-macos
//
//  Created by Simon Corsin on 10/13/20.
//

#import <Foundation/Foundation.h>

typedef id (^SCValdiMacOSFunctionBlock)(NSArray<id>* parameters);

@interface SCValdiMacOSFunction : NSObject

@property (readonly, nonatomic) void* cppInstance;

- (instancetype)initWithCppInstance:(void*)cppInstance;
- (instancetype)initWithBlock:(SCValdiMacOSFunctionBlock)block;

- (void)performWithParameters:(NSArray<id>*)parameters;

@end
