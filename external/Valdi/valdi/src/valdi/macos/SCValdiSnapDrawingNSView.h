//
//  SCValdiSnapDrawingNSView.h
//  valdi-skia-apple
//
//  Created by Simon Corsin on 6/27/20.
//

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#import "SCValdiRuntime.h"

@interface SCValdiSnapDrawingNSView : NSView

- (instancetype)initWithValdiRuntime:(SCValdiRuntime*)runtime
                           arguments:(NSArray<NSString*>*)arguments
                    componentContext:(id)componentContext;

- (instancetype)initWithValdiRuntime:(SCValdiRuntime*)runtime
                           arguments:(NSArray<NSString*>*)arguments
                    componentContext:(id)componentContext
                       componentPath:(NSString*)componentPath;

@end
