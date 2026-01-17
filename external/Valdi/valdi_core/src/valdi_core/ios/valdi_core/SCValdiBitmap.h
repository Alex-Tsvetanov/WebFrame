#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#ifdef __cplusplus
namespace Valdi {
struct BitmapInfo;
}
extern "C" {
#endif

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, SCValdiBitmapColorType) {
    SCValdiBitmapColorTypeRGBA8888,
    SCValdiBitmapColorTypeBGRA8888,
    SCValdiBitmapColorTypeAlpha8,
    SCValdiBitmapColorTypeGray8,
    SCValdiBitmapColorTypeRGBAF16,
    SCValdiBitmapColorTypeRGBAF32
};

typedef NS_ENUM(NSUInteger, SCValdiBitmapAlphaType) {
    SCValdiBitmapAlphaTypeOpaque,
    SCValdiBitmapAlphaTypePremul,
    SCValdiBitmapAlphaTypeUnpremul,
};

typedef struct SCValdiBitmapInfo {
    int bitmapWidth;
    int bitmapHeight;
    SCValdiBitmapColorType colorType;
    SCValdiBitmapAlphaType alphaType;
    NSUInteger rowBytes;
} SCValdiBitmapInfo;

@protocol SCValdiBitmap <NSObject>

- (SCValdiBitmapInfo)bitmapInfo;
- (void*)lockBytes;
- (void)unlockBytes;

@end

typedef void (^SCValdiReleaseBitmapBlock)();

SCValdiBitmapInfo SCValdiBitmapInfoCreate(int bitmapWidth,
                                          int bitmapHeight,
                                          SCValdiBitmapColorType colorType,
                                          SCValdiBitmapAlphaType alphaType,
                                          NSUInteger rowBytes);

CGBitmapInfo CGBitmapInfoFromValdiBitmapInfo(const SCValdiBitmapInfo* bitmapInfo);

id<SCValdiBitmap> SCValdiBitmapCreate(void* bitmapData,
                                      const SCValdiBitmapInfo* bitmapInfo,
                                      SCValdiReleaseBitmapBlock releaseBlock);

#ifdef __cplusplus
CGBitmapInfo CGBitmapInfoFromValdiBitmapInfoCpp(const Valdi::BitmapInfo& bitmapInfo);
#endif

NS_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif
