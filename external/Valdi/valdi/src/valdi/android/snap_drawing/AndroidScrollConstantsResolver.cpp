//
//  AndroidScrollConstantsResolver.cpp
//  valdi-android
//
//  Created by Simon Corsin on 9/26/23.
//

#include "valdi/android/snap_drawing/AndroidScrollConstantsResolver.hpp"

#include "valdi/runtime/Interfaces/IDiskCache.hpp"

#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/ScopedJNIExceptionHandler.hpp"

#include "utils/platform/JvmUtils.hpp"

#include <thread>

namespace ValdiAndroid {

constexpr const char* kCacheKey = ".scroll_physics";
constexpr const char* kGravityKey = "GRAVITY";
constexpr const char* kDecelerationRateKey = "DECELERATION_RATE";
constexpr const char* kInflexionKey = "INFLEXION";
constexpr const char* kStartTensionKey = "START_TENSION";
constexpr const char* kEndTensionKey = "END_TENSION";

static std::optional<ScrollConstants> getFromCache(const Valdi::Ref<Valdi::IDiskCache>& diskCache) {
    auto cacheKey = Valdi::Path(std::string_view(kCacheKey));

    if (!diskCache->exists(cacheKey)) {
        return std::nullopt;
    }
    auto cacheEntry = diskCache->load(cacheKey);
    if (!cacheEntry) {
        return std::nullopt;
    }
    auto result = Valdi::deserializeValue(cacheEntry.value().data(), cacheEntry.value().size());

    auto gravity = result.getMapValue(kGravityKey);
    auto inflexion = result.getMapValue(kInflexionKey);
    auto startTension = result.getMapValue(kStartTensionKey);
    auto endTension = result.getMapValue(kEndTensionKey);
    auto decelerationRate = result.getMapValue(kDecelerationRateKey);

    if (gravity.isNullOrUndefined() || inflexion.isNullOrUndefined() || startTension.isNullOrUndefined() ||
        endTension.isNullOrUndefined() || decelerationRate.isNullOrUndefined()) {
        return std::nullopt;
    }

    return ScrollConstants(gravity.toFloat(),
                           inflexion.toFloat(),
                           startTension.toFloat(),
                           endTension.toFloat(),
                           decelerationRate.toFloat());
}

static void storeInCache(const Valdi::Ref<Valdi::IDiskCache>& diskCache,
                         const ScrollConstants& scrollConstants,
                         Valdi::ILogger& logger) {
    auto cachedOutput = Valdi::Value()
                            .setMapValue(kGravityKey, Valdi::Value(scrollConstants.gravity))
                            .setMapValue(kInflexionKey, Valdi::Value(scrollConstants.inflexion))
                            .setMapValue(kStartTensionKey, Valdi::Value(scrollConstants.startTension))
                            .setMapValue(kEndTensionKey, Valdi::Value(scrollConstants.endTension))
                            .setMapValue(kDecelerationRateKey, Valdi::Value(scrollConstants.decelerationRate));

    auto cacheEntry = Valdi::serializeValue(cachedOutput);

    auto cacheKey = Valdi::Path(std::string_view(kCacheKey));
    auto storeResult = diskCache->store(cacheKey, cacheEntry->toBytesView());
    if (!storeResult) {
        VALDI_ERROR(logger, "Failed to store Scroll Constants in disk cache: {}", storeResult.error());
    }
}

static float getScrollConstant(const ScopedJNIExceptionHandler& exceptionHandler,
                               jobject overScrollerClass,
                               const JavaClass& cls,
                               const AnyJavaMethod& getDeclaredFieldMethod,
                               const char* fieldName) {
    auto parameter = toJavaObject(JavaEnv(), std::string_view(fieldName));
    auto javaFieldWrapper = getDeclaredFieldMethod.call(overScrollerClass, 1, &parameter.getValue());

    if (!exceptionHandler) {
        return 0;
    }

    auto javaField = AnyJavaField::fromReflectedField(javaFieldWrapper.getObject(), JavaValueType::Float, true);

    if (!exceptionHandler) {
        return 0;
    }

    auto value = javaField.getFieldValue(reinterpret_cast<jobject>(cls.getClass()));

    if (!exceptionHandler) {
        return 0;
    }

    return value.getFloat();
}

static std::optional<ScrollConstants> doResolveScrollConstants(const ScopedJNIExceptionHandler& exceptionHandler) {
    /**
         This code aims to read the constant values from android.widget.OverScroller$SplineOverScroller
        See:
        https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/core/java/android/widget/OverScroller.java#580
    */
    auto parameterType = Valdi::ValueSchema::string();

    auto javaReflectionClass = JavaClass::resolveOrAbort(JavaEnv(), "java/lang/Class");
    auto classForNameMethod = javaReflectionClass.getStaticMethod(
        "forName", Valdi::ValueSchema::cls(STRING_LITERAL("java/lang/Class"), false, {}), &parameterType, 1, false);

    auto overScrollerClassName =
        toJavaObject(JavaEnv(), std::string_view("android.widget.OverScroller$SplineOverScroller"));

    auto overScrollerClassJava = classForNameMethod.call(
        reinterpret_cast<jobject>(javaReflectionClass.getClass()), 1, &overScrollerClassName.getValue());

    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto overScrollerMetaclass = JavaClass::fromReflectedClass(JavaEnv(), overScrollerClassJava.getObject());
    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto overScrollerClassResult = JavaClass::resolve(JavaEnv(), "android/widget/OverScroller$SplineOverScroller");
    if (!overScrollerClassResult) {
        return std::nullopt;
    }

    const auto& overScrollerClass = overScrollerClassResult.value();

    auto getDeclaredFieldMethod =
        overScrollerMetaclass.getMethod("getDeclaredField",
                                        Valdi::ValueSchema::cls(STRING_LITERAL("java/lang/reflect/Field"), false, {}),
                                        &parameterType,
                                        1,
                                        false);
    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto gravity = getScrollConstant(
        exceptionHandler, overScrollerClassJava.getObject(), overScrollerClass, getDeclaredFieldMethod, kGravityKey);
    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto decelerationRate = getScrollConstant(exceptionHandler,
                                              overScrollerClassJava.getObject(),
                                              overScrollerClass,
                                              getDeclaredFieldMethod,
                                              kDecelerationRateKey);
    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto inflexion = getScrollConstant(
        exceptionHandler, overScrollerClassJava.getObject(), overScrollerClass, getDeclaredFieldMethod, kInflexionKey);
    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto startTension = getScrollConstant(exceptionHandler,
                                          overScrollerClassJava.getObject(),
                                          overScrollerClass,
                                          getDeclaredFieldMethod,
                                          kStartTensionKey);
    if (!exceptionHandler) {
        return std::nullopt;
    }

    auto endTension = getScrollConstant(
        exceptionHandler, overScrollerClassJava.getObject(), overScrollerClass, getDeclaredFieldMethod, kEndTensionKey);
    if (!exceptionHandler) {
        return std::nullopt;
    }

    return ScrollConstants(gravity, inflexion, startTension, endTension, decelerationRate);
}

static std::optional<ScrollConstants> resolveScrollConstants() {
    snap::utils::platform::attachJni("Valdi Scroll Constants");
    std::optional<ScrollConstants> resolvedScrollConstants;
    {
        ScopedJNIExceptionHandler exceptionHandler;
        resolvedScrollConstants = doResolveScrollConstants(exceptionHandler);
    }

    snap::utils::platform::detachJni();
    return resolvedScrollConstants;
}

static Valdi::StringBox getDeviceManufacter() {
    auto buildCls = JavaClass::resolveOrAbort(JavaEnv(), "android/os/Build");
    auto manufacturerField = buildCls.getStaticField("MANUFACTURER", Valdi::ValueSchema::string(), false);
    auto manufacturer = manufacturerField.getFieldValue(reinterpret_cast<jobject>(buildCls.getClass()));
    return toInternedString(JavaEnv(), manufacturer.getString()).lowercased();
}

ScrollConstants ScrollConstants::resolve(const Valdi::Ref<Valdi::IDiskCache>& diskCache, Valdi::ILogger& logger) {
    // Uncomment to print scroll constants to help populating the database
    // printScrollConstants();
    auto cachedValue = getFromCache(diskCache);
    if (!cachedValue) {
        VALDI_INFO(logger, "Resolving Scroll Constants");
        // We have to use a separate native thread to work around the Android reflection protection
        // See: https://github.com/ChickenHook/RestrictionBypass
        std::thread resolverThread([&]() { cachedValue = resolveScrollConstants(); });
        resolverThread.join();

        if (cachedValue) {
            VALDI_INFO(logger, "Scroll Constants resolved succesfully");
        } else {
            VALDI_ERROR(logger, "Scroll Constants failed to resolve. Falling back on default values");
            auto manufacturer = getDeviceManufacter();
            if (manufacturer == "samsung") {
                cachedValue = ScrollConstants(2000.0, 0.220000, 0.5, 1.0, 2.358202f);
            } else {
                // Keep default values
                // The spline scroll physics logic will fallback on the default Android values when passing back 0
                cachedValue = ScrollConstants(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        storeInCache(diskCache, *cachedValue, logger);
    }

    return *cachedValue;
}

} // namespace ValdiAndroid
