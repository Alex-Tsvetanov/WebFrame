#import "valdi_core/SCValdiSnapDrawingFontManager.h"
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class SCValdiCppObject;
@protocol SCSnapDrawingRuntime;

typedef NS_ENUM(NSUInteger, SCSnapDrawingBitmapColorType) {
    SCSnapDrawingBitmapColorTypeRGBA8888,
    SCSnapDrawingBitmapColorTypeBGRA8888,
    SCSnapDrawingBitmapColorTypeAlpha8,
    SCSnapDrawingBitmapColorTypeGray8,
    SCSnapDrawingBitmapColorTypeRGBAF16,
    SCSnapDrawingBitmapColorTypeRGBAF32
};

typedef struct SCSnapDrawingBitmapInfo {
    int bitmapWidth;
    int bitmapHeight;
    SCSnapDrawingBitmapColorType colorType;
    NSUInteger rowBytes;
} SCSnapDrawingBitmapInfo;

@interface SCSnapDrawingAnimatedImage : NSObject

@property (readonly, nonatomic) NSTimeInterval duration;
@property (readonly, nonatomic) double frameRate;
@property (readonly, nonatomic) CGSize size;

+ (nullable instancetype)imageWithRuntime:(id<SCSnapDrawingRuntime>)runtime data:(NSData*)data error:(NSError**)error;
+ (nullable instancetype)imageWithFontManager:(SCValdiSnapDrawingFontManager*)fontManager
                                         data:(NSData*)data
                                        error:(NSError**)error;

+ (instancetype)imageWithCppObject:(SCValdiCppObject*)cppObject;

/**
 * Draw the image into the given Bitmap at the given time.
 * @param bitmapData The bitmap that the frame should be rasterized into
 * @param bitmapInfo  the information about the bitmap format
 * @param drawBounds  the coordinates and size within the bitmap where the image should be rendered
 * @param timeSeconds the time in the seconds that should be rendered
 */
- (void)drawInBitmap:(void*)bitmapData
          bitmapInfo:(const SCSnapDrawingBitmapInfo*)bitmapInfo
          drawBounds:(CGRect)drawBounds
              atTime:(NSTimeInterval)timeSeconds;

@end

NS_ASSUME_NONNULL_END
