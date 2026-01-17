#import "valdi/ios/SnapDrawing/AnimatedImage/SCSnapDrawingAnimatedImage.h"
#import "valdi/ios/SnapDrawing/SCSnapDrawingUIView.h"
#import "valdi_core/SCMacros.h"

NS_ASSUME_NONNULL_BEGIN

@class SCValdiCppObject;

typedef void (^SCSnapDrawingAnimatedImageProgressHandler)(NSTimeInterval currentTime, NSTimeInterval duration);

@interface SCSnapDrawingAnimatedImageView : SCSnapDrawingUIView

// Use the superclass initializer.
VALDI_NO_VIEW_INIT

/**
 * Set the image to render.
 * If shouldDrawFlipped is true, the image will be drawn horizontally flipped.
 */
- (void)setImage:(SCSnapDrawingAnimatedImage*)image shouldDrawFlipped:(BOOL)shouldDrawFlipped;

/**
 * Clears any previously set image.
 */
- (void)clearImage;

/**
 * Sets the speed ratio at which the image animation should run, 0 is paused, 1 means
 * the animation runs at normal speed, 0.5 at half speed, -1 the animation will run
 * in reverse.
 * Default is 0.
 */
- (void)setAdvanceRate:(double)advanceRate;

/**
 * Specify whether the animation should loop back to the beginning when reaching the end.
 */
- (void)setShouldLoop:(BOOL)shouldLoop;

/**
 * Set the current time in seconds for the animation.
 */
- (void)setCurrentTime:(NSTimeInterval)currentTime;

- (void)setAnimationStartTime:(NSTimeInterval)startTime;

- (void)setAnimationEndTime:(NSTimeInterval)endTime;

/**
 * Set the progress handler that will be called as the animation progresses.
 */
- (void)setProgressHandler:(nullable SCSnapDrawingAnimatedImageProgressHandler)progressHandler;

- (void)setObjectFit:(nullable NSString*)objectFit;

@end

NS_ASSUME_NONNULL_END
