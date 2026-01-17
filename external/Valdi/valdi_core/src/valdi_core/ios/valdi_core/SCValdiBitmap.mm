#import "valdi_core/SCValdiBitmap.h"
#include "SCValdiBitmap.h"
#import "valdi_core/SCValdiMarshallableObject.h"
#import "valdi_core/SCValdiMarshallableObjectDescriptor.h"
#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#import "valdi_core/SCValdiWrappedValue+Private.h"

static Valdi::ColorType toCppColorType(SCValdiBitmapColorType colorType) {
    switch (colorType) {
        case SCValdiBitmapColorTypeRGBA8888:
            return Valdi::ColorTypeRGBA8888;
        case SCValdiBitmapColorTypeBGRA8888:
            return Valdi::ColorTypeBGRA8888;
        case SCValdiBitmapColorTypeAlpha8:
            return Valdi::ColorTypeAlpha8;
        case SCValdiBitmapColorTypeGray8:
            return Valdi::ColorTypeGray8;
        case SCValdiBitmapColorTypeRGBAF16:
            return Valdi::ColorTypeRGBAF16;
        case SCValdiBitmapColorTypeRGBAF32:
            return Valdi::ColorTypeRGBAF32;
    }
}

static SCValdiBitmapColorType fromCppColorType(Valdi::ColorType colorType) {
    switch (colorType) {
        case Valdi::ColorTypeUnknown:
        case Valdi::ColorTypeRGBA8888:
            return SCValdiBitmapColorTypeRGBA8888;
        case Valdi::ColorTypeBGRA8888:
            return SCValdiBitmapColorTypeBGRA8888;
        case Valdi::ColorTypeAlpha8:
            return SCValdiBitmapColorTypeAlpha8;
        case Valdi::ColorTypeGray8:
            return SCValdiBitmapColorTypeGray8;
        case Valdi::ColorTypeRGBAF16:
            return SCValdiBitmapColorTypeRGBAF16;
        case Valdi::ColorTypeRGBAF32:
            return SCValdiBitmapColorTypeRGBAF32;
    }
}

static Valdi::AlphaType toCppAlphaType(SCValdiBitmapAlphaType alphaType) {
    switch (alphaType) {
        case SCValdiBitmapAlphaTypeOpaque:
            return Valdi::AlphaTypeOpaque;
        case SCValdiBitmapAlphaTypePremul:
            return Valdi::AlphaTypePremul;
        case SCValdiBitmapAlphaTypeUnpremul:
            return Valdi::AlphaTypeUnpremul;
    }
}

static SCValdiBitmapAlphaType fromCppAlphaType(Valdi::AlphaType alphaType) {
    switch (alphaType) {
        case Valdi::AlphaTypeOpaque:
            return SCValdiBitmapAlphaTypeOpaque;
        case Valdi::AlphaTypePremul:
            return SCValdiBitmapAlphaTypePremul;
        case Valdi::AlphaTypeUnpremul:
            return SCValdiBitmapAlphaTypeUnpremul;
    }
}

@interface SCValdiBitmap: SCValdiProxyMarshallableObject

@end

@implementation SCValdiBitmap

+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor
{
    return SCValdiMarshallableObjectDescriptorMake(nil, nil, nil, SCValdiMarshallableObjectTypeUntyped);
}

@end

@interface SCValdiBridgedBitmap: SCValdiWrappedValue <SCValdiBitmap>

@end

@implementation SCValdiBridgedBitmap

- (SCValdiBitmapInfo)bitmapInfo
{
    auto info = self.value.getTypedRef<Valdi::IBitmap>()->getInfo();
    return SCValdiBitmapInfoCreate(info.width, info.height, fromCppColorType(info.colorType), fromCppAlphaType(info.alphaType), info.rowBytes);
}

- (void *)lockBytes
{
    return self.value.getTypedRef<Valdi::IBitmap>()->lockBytes();
}

- (void)unlockBytes
{
    self.value.getTypedRef<Valdi::IBitmap>()->unlockBytes();
}

- (NSInteger)pushToValdiMarshaller:(SCValdiMarshallerRef)marshaller
{
    return SCValdiMarshallerPushOpaque(marshaller, self);
}

@end

namespace ValdiIOS {

class BridgedBitmap: public Valdi::IBitmap {
public:
    BridgedBitmap(void* bitmapData, const SCValdiBitmapInfo *bitmapInfo, SCValdiReleaseBitmapBlock releaseBlock): _bitmapData(bitmapData), _bitmapInfo(*bitmapInfo), _releaseBlock(releaseBlock) {}

    ~BridgedBitmap() override {
        if (_releaseBlock) {
            _releaseBlock();
        }
    }

    void dispose() override {
        auto releaseBlock = _releaseBlock;
        _releaseBlock = nil;
        if (releaseBlock) {
            releaseBlock();
        }
    }

    Valdi::BitmapInfo getInfo() const override {
        return Valdi::BitmapInfo(_bitmapInfo.bitmapWidth, _bitmapInfo.bitmapHeight, toCppColorType(_bitmapInfo.colorType), toCppAlphaType(_bitmapInfo.alphaType), _bitmapInfo.rowBytes);
    }

    void* lockBytes() override {
        return _bitmapData;
    }

    void unlockBytes() override {}

private:
    void* _bitmapData;
    SCValdiBitmapInfo _bitmapInfo;
    SCValdiReleaseBitmapBlock _releaseBlock;
};

}

FOUNDATION_EXPORT SCValdiBitmapInfo SCValdiBitmapInfoCreate(int bitmapWidth,
                                                                  int bitmapHeight,
                                                                  SCValdiBitmapColorType colorType,
                                                                  SCValdiBitmapAlphaType alphaType,
                                                                  NSUInteger rowBytes)
{
    SCValdiBitmapInfo out;
    out.bitmapWidth = bitmapWidth;
    out.bitmapHeight = bitmapHeight;
    out.colorType = colorType;
    out.alphaType = alphaType;
    out.rowBytes = rowBytes;

    return out;
}

FOUNDATION_EXPORT id<SCValdiBitmap> SCValdiBitmapCreate(void* bitmapData,
                                                              const SCValdiBitmapInfo *bitmapInfo,
                                                              SCValdiReleaseBitmapBlock releaseBlock)
{
    auto bitmap = Valdi::makeShared<ValdiIOS::BridgedBitmap>(bitmapData, bitmapInfo, releaseBlock);
    return [[SCValdiBridgedBitmap alloc] initWithValue:Valdi::Value(bitmap)];
}

static CGBitmapInfo CGBitmapInfoFromValdiColorTypeAndAlphaType(SCValdiBitmapColorType colorType, SCValdiBitmapAlphaType alphaType)
{
  switch (colorType) {
        case SCValdiBitmapColorTypeRGBA8888:
            switch (alphaType) {
                case SCValdiBitmapAlphaTypeOpaque:
                    return kCGBitmapByteOrder32Big;
                case SCValdiBitmapAlphaTypePremul:
                    return (uint32_t)kCGBitmapByteOrder32Big | (uint32_t)kCGImageAlphaPremultipliedLast;
                case SCValdiBitmapAlphaTypeUnpremul:
                    return (uint32_t)kCGBitmapByteOrder32Big | (uint32_t)kCGImageAlphaLast;
            }
        case SCValdiBitmapColorTypeBGRA8888:
            switch (alphaType) {
                case SCValdiBitmapAlphaTypeOpaque:
                    return kCGBitmapByteOrder32Little;
                case SCValdiBitmapAlphaTypePremul:
                    return (uint32_t)kCGBitmapByteOrder32Little | (uint32_t)kCGImageAlphaPremultipliedFirst;
                case SCValdiBitmapAlphaTypeUnpremul:
                    return (uint32_t)kCGBitmapByteOrder32Little | (uint32_t)kCGImageAlphaFirst;
            }
        case SCValdiBitmapColorTypeAlpha8:
            return kCGImageAlphaOnly;
        case SCValdiBitmapColorTypeGray8:
            // Not verified
            return kCGImageAlphaOnly;
        case SCValdiBitmapColorTypeRGBAF16:
            // Not verified
            return (uint32_t)kCGImageAlphaNoneSkipLast | (uint32_t)kCGBitmapFloatComponents;
        case SCValdiBitmapColorTypeRGBAF32:
            // Not verified
            return (uint32_t)kCGImageAlphaNoneSkipLast | (uint32_t)kCGBitmapFloatComponents;
    }
}

FOUNDATION_EXPORT CGBitmapInfo CGBitmapInfoFromValdiBitmapInfoCpp(const Valdi::BitmapInfo& bitmapInfo)
{
    return CGBitmapInfoFromValdiColorTypeAndAlphaType(fromCppColorType(bitmapInfo.colorType), fromCppAlphaType(bitmapInfo.alphaType));
}

FOUNDATION_EXPORT CGBitmapInfo CGBitmapInfoFromValdiBitmapInfo(const SCValdiBitmapInfo *bitmapInfo)
{
    return CGBitmapInfoFromValdiColorTypeAndAlphaType(bitmapInfo->colorType, bitmapInfo->alphaType);
}
