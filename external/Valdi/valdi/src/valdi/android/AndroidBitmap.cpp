#include "valdi/android/AndroidBitmap.hpp"
#include "valdi_core/jni/JavaCache.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

#include <android/bitmap.h>

namespace ValdiAndroid {

static Valdi::ColorType getColorType(int32_t format) {
    if (format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        return Valdi::ColorTypeRGBA8888;
    } else if (format == ANDROID_BITMAP_FORMAT_A_8) {
        return Valdi::ColorTypeAlpha8;
    }
    return Valdi::ColorTypeUnknown;
}

static Valdi::AlphaType getAlphaType([[maybe_unused]] int32_t flags) {
#if defined(__ANDROID_API__) && __ANDROID_API__ >= 30
    // TODO(simon): The NDK doc doesn't specify whether we should first
    // shift and mask or the opposite? I was not able to find a code sample of this.
    // The shift value is currently 0 so it doesn't matter at the moment but it could
    // later change.
    auto alphaType = (flags >> ANDROID_BITMAP_FLAGS_ALPHA_SHIFT) & ANDROID_BITMAP_FLAGS_ALPHA_MASK;
    if (alphaType == ANDROID_BITMAP_FLAGS_ALPHA_PREMUL) {
        return Valdi::AlphaTypePremul;
    } else if (alphaType == ANDROID_BITMAP_FLAGS_ALPHA_OPAQUE) {
        return Valdi::AlphaTypeOpaque;
    } else if (alphaType == ANDROID_BITMAP_FLAGS_ALPHA_UNPREMUL) {
        return Valdi::AlphaTypeUnpremul;
    }

    return Valdi::AlphaTypeOpaque;
#else
    return Valdi::AlphaTypePremul;
#endif
}

AndroidBitmap::AndroidBitmap(const JavaObject& bitmap) : _bitmap(bitmap, "Bitmap") {}

AndroidBitmap::~AndroidBitmap() = default;

void AndroidBitmap::dispose() {
    _bitmap = GlobalRefJavaObjectBase();
}

Valdi::BitmapInfo AndroidBitmap::getInfo() const {
    auto jBitmap = _bitmap.get();
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(JavaEnv::getUnsafeEnv(), jBitmap.get(), &info);

    return Valdi::BitmapInfo(info.width, info.height, getColorType(info.format), getAlphaType(info.flags), info.stride);
}

void* AndroidBitmap::lockBytes() {
    auto jBitmap = _bitmap.get();
    void* addrPtr = nullptr;
    AndroidBitmap_lockPixels(JavaEnv::getUnsafeEnv(), jBitmap.get(), &addrPtr);
    return addrPtr;
}

void AndroidBitmap::unlockBytes() {
    auto jBitmap = _bitmap.get();
    AndroidBitmap_unlockPixels(JavaEnv::getUnsafeEnv(), jBitmap.get());
}

JavaObject AndroidBitmap::getJavaBitmap() const {
    return _bitmap.toObject();
}

static GlobalRefJavaObjectBase getBitmapConfigForName(const char* name) {
    auto& cache = JavaEnv::getCache();
    auto jConfigName = toJavaObject(JavaEnv(), std::string_view(name));
    return GlobalRefJavaObjectBase(
        cache.getBitmapConfigValueOfMethod().call(cache.getBitmapConfigClass().getClass(), jConfigName),
        "BitmapConfig");
}

Valdi::Result<Valdi::Ref<AndroidBitmap>> AndroidBitmap::make(const Valdi::BitmapInfo& info) {
    const GlobalRefJavaObjectBase* config = nullptr;
    switch (info.colorType) {
        case Valdi::ColorTypeBGRA8888: {
            static auto kConfig = getBitmapConfigForName("ARGB_8888");
            config = &kConfig;
            break;
        }
        case Valdi::ColorTypeAlpha8: {
            static auto kConfig = getBitmapConfigForName("ALPHA_8");
            config = &kConfig;
            break;
        }
        default:
            return Valdi::Error("Unsupported Bitmap color type for Android Bitmap");
    }

    auto& cache = JavaEnv::getCache();
    auto jBitmap = cache.getBitmapCreateBitmapMethod().call(cache.getBitmapClass().getClass(),
                                                            static_cast<int32_t>(info.width),
                                                            static_cast<int32_t>(info.height),
                                                            BitmapConfigType(JavaEnv(), config->get()));

    if (jBitmap.isNull()) {
        return Valdi::Error("Bitmap.createBitmap returned null");
    }

    // // Ensure premultiplied state matches alphaType expectation
    // if (info.alphaType == Valdi::AlphaTypePremul || info.alphaType == Valdi::AlphaTypeOpaque) {
    //     cache.getBitmapSetPremultipliedMethod().call(jBitmap, true);
    // } else if (info.alphaType == Valdi::AlphaTypeUnpremul) {
    //     cache.getBitmapSetPremultipliedMethod().call(jBitmap, false);
    // }

    return Valdi::makeShared<AndroidBitmap>(jBitmap);
}

AndroidBitmapHandler::AndroidBitmapHandler(JavaEnv env, jobject bitmapHandler, bool isOwned)
    : AndroidBitmap(JavaEnv::getCache().getBitmapHandlerGetBitmapMethod().call(bitmapHandler)),
      _bitmapHandlerRef(env, bitmapHandler, "BitmapHandler"),
      _isOwned(isOwned) {}

AndroidBitmapHandler::~AndroidBitmapHandler() {
    if (_isOwned) {
        JavaEnv::getCache().getBitmapHandlerReleaseMethod().call(_bitmapHandlerRef.toObject());
    }
}

} // namespace ValdiAndroid
