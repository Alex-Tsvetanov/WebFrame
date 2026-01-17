#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaCache.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

#include <fbjni/fbjni.h>

#include "snap_drawing/cpp/Drawing/GraphicsContext/BitmapGraphicsContext.hpp"
#include "snap_drawing/cpp/Layers/AnimatedImageLayer.hpp"
#include "snap_drawing/cpp/Utils/AnimatedImage.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi/android/AndroidBitmap.hpp"
#include "valdi/android/snap_drawing/SnapDrawingLayerRootHost.hpp"
#include "valdi/snap_drawing/Layers/Classes/AnimatedImageLayerClass.hpp"
#include "valdi/snap_drawing/Runtime.hpp"

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class AnimatedImageViewJNI : public fbjni::JavaClass<AnimatedImageViewJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/views/AnimatedImageView;";

    static Valdi::Ref<ValdiAndroid::SnapDrawingLayerRootHost> getHost(jlong ptr) {
        return Valdi::unsafeBridge<ValdiAndroid::SnapDrawingLayerRootHost>(reinterpret_cast<void*>(ptr));
    }

    static Valdi::Ref<snap::drawing::AnimatedImageLayer> getAnimatedImageLayer(jlong ptr) {
        return Valdi::castOrNull<snap::drawing::AnimatedImageLayer>(getHost(ptr)->getLayerRoot()->getContentLayer());
    }

    // NOLINTNEXTLINE
    static void nativeSetAnimatedImageLayerAsContentLayer(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr) {
        auto host = getHost(ptr);
        host->getLayerRoot()->setContentLayer(
            snap::drawing::makeLayer<snap::drawing::AnimatedImageLayer>(host->getLayerRoot()->getResources()),
            snap::drawing::ContentLayerSizingModeMatchSize);
    }

    // NOLINTNEXTLINE
    static void nativeSetImage(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                               jlong ptr,
                               jlong imageHandle,
                               jboolean shouldDrawFlipped) {
        auto image = ValdiAndroid::valueFromJavaCppHandle(imageHandle).getTypedRef<snap::drawing::AnimatedImage>();
        auto imageLayer = getAnimatedImageLayer(ptr);
        imageLayer->setImage(image);
        imageLayer->setShouldFlip(static_cast<bool>(shouldDrawFlipped));
    }

    // NOLINTNEXTLINE
    static void nativeSetAdvanceRate(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jdouble advanceRate) {
        getAnimatedImageLayer(ptr)->setAdvanceRate(static_cast<double>(advanceRate));
    }

    // NOLINTNEXTLINE
    static void nativeSetShouldLoop(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jboolean shouldLoop) {
        getAnimatedImageLayer(ptr)->setShouldLoop(static_cast<bool>(shouldLoop));
    }

    // NOLINTNEXTLINE
    static void nativeSetCurrentTime(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jdouble currentTime) {
        getAnimatedImageLayer(ptr)->setCurrentTime(
            snap::drawing::Duration::fromSeconds(static_cast<double>(currentTime)));
    }

    // NOLINTNEXTLINE
    static void nativeSetAnimationStartTime(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jdouble startTime) {
        getAnimatedImageLayer(ptr)->setAnimationStartTime(
            snap::drawing::Duration::fromSeconds(static_cast<double>(startTime)));
    }

    // NOLINTNEXTLINE
    static void nativeSetAnimationEndTime(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jdouble endTime) {
        getAnimatedImageLayer(ptr)->setAnimationEndTime(
            snap::drawing::Duration::fromSeconds(static_cast<double>(endTime)));
    }

    // NOLINTNEXTLINE
    static void nativeSetOnProgress(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jobject onProgress) {
        auto onProgressValue = toValue(JavaEnv(), ::djinni::LocalRef<jobject>(onProgress));
        snap::drawing::AnimatedImageLayerListenerImpl::setCallbackOnLayer(*getAnimatedImageLayer(ptr),
                                                                          onProgressValue.getFunctionRef());
    }

    // NOLINTNEXTLINE
    static void nativeSetScaleType(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong ptr, jstring scaleType) {
        auto scaleTypeStd = toStdString(JavaEnv(), scaleType);
        snap::drawing::FittingSizeMode fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFit;

        if (scaleTypeStd == "none") {
            fittingSizeMode = snap::drawing::FittingSizeModeCenter;
        } else if (scaleTypeStd == "fill") {
            fittingSizeMode = snap::drawing::FittingSizeModeFill;
        } else if (scaleTypeStd == "cover") {
            fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFill;
        } else if (scaleTypeStd == "contain") {
            fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFit;
        }

        getAnimatedImageLayer(ptr)->setFittingSizeMode(fittingSizeMode);
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeSetAnimatedImageLayerAsContentLayer",
                             AnimatedImageViewJNI::nativeSetAnimatedImageLayerAsContentLayer),
            makeNativeMethod("nativeSetImage", AnimatedImageViewJNI::nativeSetImage),
            makeNativeMethod("nativeSetAdvanceRate", AnimatedImageViewJNI::nativeSetAdvanceRate),
            makeNativeMethod("nativeSetShouldLoop", AnimatedImageViewJNI::nativeSetShouldLoop),
            makeNativeMethod("nativeSetCurrentTime", AnimatedImageViewJNI::nativeSetCurrentTime),
            makeNativeMethod("nativeSetAnimationStartTime", AnimatedImageViewJNI::nativeSetAnimationStartTime),
            makeNativeMethod("nativeSetAnimationEndTime", AnimatedImageViewJNI::nativeSetAnimationEndTime),
            makeNativeMethod("nativeSetOnProgress", AnimatedImageViewJNI::nativeSetOnProgress),
            makeNativeMethod("nativeSetScaleType", AnimatedImageViewJNI::nativeSetScaleType),
        });
    }
};

class AnimatedImageJNI : public fbjni::JavaClass<AnimatedImageJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/snapdrawing/AnimatedImage;";

    // NOLINTNEXTLINE
    static jobject nativeParse(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                               jlong snapDrawingRuntimeOrFontManagerHandle,
                               jbyteArray imageData,
                               jboolean isFontManager) {
        Valdi::Ref<snap::drawing::IFontManager> fontManager;

        if (static_cast<bool>(isFontManager)) {
            fontManager = ValdiAndroid::valueFromJavaCppHandle(snapDrawingRuntimeOrFontManagerHandle)
                              .getTypedRef<snap::drawing::IFontManager>();
            SC_ASSERT_NOTNULL(fontManager);
        } else {
            auto runtime = Valdi::unsafeBridge<snap::drawing::Runtime>(
                reinterpret_cast<void*>(snapDrawingRuntimeOrFontManagerHandle));
            SC_ASSERT_NOTNULL(runtime);
            fontManager = runtime->getResources()->getFontManager();
        }

        auto imageBytes = toByteArray(JavaEnv(), imageData);
        auto image = snap::drawing::AnimatedImage::make(fontManager, imageBytes.data(), imageBytes.size());
        if (!image) {
            JavaEnv::getUnsafeEnv()->ThrowNew(JavaEnv::getCache().getValdiExceptionClass().getClass(),
                                              image.error().toString().c_str());
            return nullptr;
        }

        return newJavaObjectWrappingValue(JavaEnv(), Valdi::Value(image.value())).releaseObject();
    }

    // NOLINTNEXTLINE
    static Valdi::Ref<snap::drawing::AnimatedImage> getAnimatedImageFromHandle(jlong nativeHandle) {
        return ValdiAndroid::valueFromJavaCppHandle(nativeHandle).getTypedRef<snap::drawing::AnimatedImage>();
    }

    // NOLINTNEXTLINE
    static void nativeDrawInBitmap(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong nativeHandle,
                                   jobject bitmap,
                                   jint x,
                                   jint y,
                                   jint width,
                                   jint height,
                                   jdouble timeSeconds) {
        auto image = getAnimatedImageFromHandle(nativeHandle);

        auto androidBitmap = Valdi::makeShared<AndroidBitmap>(JavaObject(JavaEnv(), bitmap));
        snap::drawing::BitmapGraphicsContext graphicsContext;
        auto surface = graphicsContext.createBitmapSurface(androidBitmap);
        auto result = surface->prepareCanvas();
        if (!result) {
            throwJavaValdiException(JavaEnv::getUnsafeEnv(), result.error());
            return;
        }

        image->drawInCanvas(result.value(),
                            snap::drawing::Rect::makeXYWH(static_cast<snap::drawing::Scalar>(x),
                                                          static_cast<snap::drawing::Scalar>(y),
                                                          static_cast<snap::drawing::Scalar>(width),
                                                          static_cast<snap::drawing::Scalar>(height)),
                            snap::drawing::Duration::fromSeconds(static_cast<double>(timeSeconds)));

        surface->flush();
    }

    // NOLINTNEXTLINE
    static jdouble nativeGetDuration(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong nativeHandle) {
        auto image = getAnimatedImageFromHandle(nativeHandle);
        return image == nullptr ? 0.0 : image->getDuration().seconds();
    }

    // NOLINTNEXTLINE
    static jdouble nativeGetFrameRate(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong nativeHandle) {
        auto image = getAnimatedImageFromHandle(nativeHandle);
        return image == nullptr ? 0.0 : image->getFrameRate();
    }

    // NOLINTNEXTLINE
    static jobject nativeGetSize(fbjni::alias_ref<fbjni::JClass> /* clazz */, jlong nativeHandle) {
        auto image = getAnimatedImageFromHandle(nativeHandle);

        if (image == nullptr) {
            return nullptr;
        }

        auto imageSize = image->getSize();
        return toJavaFloatArray(JavaEnv(), {imageSize.width, imageSize.height}).releaseObject();
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativeParse", AnimatedImageJNI::nativeParse),
            makeNativeMethod("nativeDrawInBitmap", AnimatedImageJNI::nativeDrawInBitmap),
            makeNativeMethod("nativeGetDuration", AnimatedImageJNI::nativeGetDuration),
            makeNativeMethod("nativeGetFrameRate", AnimatedImageJNI::nativeGetFrameRate),
            makeNativeMethod("nativeGetSize", AnimatedImageJNI::nativeGetSize),
        });
    }
};

} // namespace ValdiAndroid
