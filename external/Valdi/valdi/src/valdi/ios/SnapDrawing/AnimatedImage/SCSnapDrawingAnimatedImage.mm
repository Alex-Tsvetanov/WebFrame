#import "valdi/ios/SnapDrawing/AnimatedImage/SCSnapDrawingAnimatedImage.h"
#import "valdi/ios/SnapDrawing/AnimatedImage/SCSnapDrawingAnimatedImage+CPP.h"
#import "valdi/snap_drawing/Runtime.hpp"
#import "valdi_core/SCSnapDrawingRuntime.h"
#import "valdi_core/SCValdiObjCConversionUtils.h"

#include "snap_drawing/cpp/Drawing/GraphicsContext/BitmapGraphicsContext.hpp"

class BridgedBitmap: public Valdi::IBitmap {
public:
    BridgedBitmap(void *bitmap, const SCSnapDrawingBitmapInfo &bitmapInfo): _bitmap(bitmap), _bitmapInfo(bitmapInfo) {}

    ~BridgedBitmap() = default;

    void dispose() override {}

    Valdi::BitmapInfo getInfo() const final {
        return Valdi::BitmapInfo(_bitmapInfo.bitmapWidth, _bitmapInfo.bitmapHeight, toValdiColorType(), Valdi::AlphaTypeOpaque, _bitmapInfo.rowBytes);
    }

    void* lockBytes() final {
        return _bitmap;
    }

    void unlockBytes() final {}

private:
    void *_bitmap;
    SCSnapDrawingBitmapInfo _bitmapInfo;

    Valdi::ColorType toValdiColorType() const {
        switch (_bitmapInfo.colorType) {
            case SCSnapDrawingBitmapColorTypeRGBA8888:
                return Valdi::ColorType::ColorTypeRGBA8888;
            case SCSnapDrawingBitmapColorTypeBGRA8888:
                return Valdi::ColorType::ColorTypeBGRA8888;
            case SCSnapDrawingBitmapColorTypeAlpha8:
                return Valdi::ColorType::ColorTypeAlpha8;
            case SCSnapDrawingBitmapColorTypeGray8:
                return Valdi::ColorType::ColorTypeGray8;
            case SCSnapDrawingBitmapColorTypeRGBAF16:
                return Valdi::ColorType::ColorTypeRGBAF16;
            case SCSnapDrawingBitmapColorTypeRGBAF32:
                return Valdi::ColorType::ColorTypeRGBAF32;
        }
    }
};

@implementation SCSnapDrawingAnimatedImage

- (instancetype)initWithImage:(Valdi::Ref<snap::drawing::AnimatedImage>)animatedImage
{
    self = [super init];
    if (self) {
        _animatedImage = animatedImage;
    }
    return self;
}

- (NSTimeInterval)duration
{
    return _animatedImage->getDuration().seconds();
}

- (double)frameRate
{
    return _animatedImage->getFrameRate();
}

- (CGSize)size
{
    const auto &size = _animatedImage->getSize();
    return CGSizeMake(static_cast<CGFloat>(size.width), static_cast<CGFloat>(size.height));
}

static SCSnapDrawingAnimatedImage *doCreateAnimatedImage(const Valdi::Ref<snap::drawing::IFontManager> &fontManager, NSData *data, NSError **error)
{
    const uint8_t* imageBytes = reinterpret_cast<const uint8_t*>(data.bytes);
    auto animatedImage = snap::drawing::AnimatedImage::make(fontManager, imageBytes, data.length);
    if (animatedImage) {
        return [[SCSnapDrawingAnimatedImage alloc] initWithImage:animatedImage.moveValue()];
    } else {
        *error = [[NSError alloc] initWithDomain:@"" code:0 userInfo:@{NSLocalizedFailureReasonErrorKey: ValdiIOS::NSStringFromSTDStringView(animatedImage.moveError().toString())}];
        return nil;
    }
}

+ (nullable instancetype)imageWithRuntime:(id<SCSnapDrawingRuntime>)runtime data:(NSData *)data error:(NSError **)error
{
    auto cppRuntime = reinterpret_cast<snap::drawing::Runtime *>([runtime handle]);

    return doCreateAnimatedImage(cppRuntime->getResources()->getFontManager(), data, error);
}

+ (nullable instancetype)imageWithFontManager:(SCValdiSnapDrawingFontManager *)fontManager data:(NSData*)data error:(NSError**)error
{
    Valdi::Value out;
    [fontManager valdi_toNative:&out];

    return doCreateAnimatedImage(out.getTypedRef<snap::drawing::IFontManager>(), data, error);
}

+ (instancetype)imageWithCppObject:(SCValdiCppObject *)cppObject
{
    return [[SCSnapDrawingAnimatedImage alloc] initWithImage:Valdi::castOrNull<snap::drawing::AnimatedImage>(cppObject.ref)];
}

- (void)drawInBitmap:(void *)bitmapData
          bitmapInfo:(const SCSnapDrawingBitmapInfo *)bitmapInfo
          drawBounds:(CGRect)drawBounds
              atTime:(NSTimeInterval)timeSeconds
{
    auto bitmap = Valdi::makeShared<BridgedBitmap>(bitmapData, *bitmapInfo);
    snap::drawing::BitmapGraphicsContext graphicsContext;
    auto surface = graphicsContext.createBitmapSurface(bitmap);
    auto result = surface->prepareCanvas();
    if (!result) {
        [NSException raise:NSInternalInconsistencyException format:@"%@", ValdiIOS::NSStringFromSTDStringView(result.error().toString())];
    }

    _animatedImage->drawInCanvas(result.value(),
                               snap::drawing::Rect::makeXYWH(drawBounds.origin.x, drawBounds.origin.y, drawBounds.size.width, drawBounds.size.height),
                               snap::drawing::Duration::fromSeconds(timeSeconds));

    surface->flush();
}

@end
