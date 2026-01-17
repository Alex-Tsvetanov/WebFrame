//
//  BitmapNativeModuleFactory.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/17/24.
//

#include "valdi/snap_drawing/Modules/BitmapNativeModuleFactory.hpp"
#include "snap_drawing/cpp/Utils/Bitmap.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"
#include "utils/encoding/Base64Utils.hpp"
#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Utils/BitmapWithBuffer.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithMethod.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace snap::drawing {

BitmapNativeModuleFactory::BitmapNativeModuleFactory() = default;

BitmapNativeModuleFactory::~BitmapNativeModuleFactory() = default;

Valdi::StringBox BitmapNativeModuleFactory::getModulePath() {
    return STRING_LITERAL("drawing/src/BitmapNative");
}

Valdi::Value BitmapNativeModuleFactory::loadModule() {
    Valdi::Value out;

    Valdi::ValueFunctionMethodBinder<BitmapNativeModuleFactory> binder(this, out);
    binder.bind("createNativeBitmap", &BitmapNativeModuleFactory::createNativeBitmap);
    binder.bind("decodeNativeBitmap", &BitmapNativeModuleFactory::decodeNativeBitmap);
    binder.bind("disposeNativeBitmap", &BitmapNativeModuleFactory::disposeNativeBitmap);
    binder.bind("encodeNativeBitmap", &BitmapNativeModuleFactory::encodeNativeBitmap);
    binder.bind("lockNativeBitmap", &BitmapNativeModuleFactory::lockNativeBitmap);
    binder.bind("unlockNativeBitmap", &BitmapNativeModuleFactory::unlockNativeBitmap);
    binder.bind("getNativeBitmapInfo", &BitmapNativeModuleFactory::getNativeBitmapInfo);
    return out;
}

Valdi::Value BitmapNativeModuleFactory::createNativeBitmap(const Valdi::ValueFunctionCallContext& callContext) {
    auto width = callContext.getParameterAsInt(0);
    auto height = callContext.getParameterAsInt(1);
    auto colorType = callContext.getParameterAsInt(2);
    auto alphaType = callContext.getParameterAsInt(3);
    auto rowBytes = callContext.getParameterAsInt(4);

    if (colorType < 0 || colorType > static_cast<int32_t>(Valdi::ColorTypeRGBAF32)) {
        callContext.getExceptionTracker().onError("Invalid colorType");
        return Valdi::Value();
    }

    if (alphaType < 0 || alphaType > static_cast<int32_t>(Valdi::AlphaTypeUnpremul)) {
        callContext.getExceptionTracker().onError("Invalid alphaType");
        return Valdi::Value();
    }

    Valdi::BitmapInfo bitmapInfo(width,
                                 height,
                                 static_cast<Valdi::ColorType>(colorType),
                                 static_cast<Valdi::AlphaType>(alphaType),
                                 static_cast<size_t>(rowBytes));

    auto existingBuffer = callContext.getParameter(5);
    if (existingBuffer.isNullOrUndefined()) {
        auto bitmap = Bitmap::make(bitmapInfo);
        if (!bitmap) {
            callContext.getExceptionTracker().onError(bitmap.moveError());
            return Valdi::Value();
        }

        return Valdi::Value(bitmap.value());
    } else {
        auto typedArray = existingBuffer.checkedTo<Ref<Valdi::ValueTypedArray>>(callContext.getExceptionTracker());
        if (typedArray == nullptr) {
            return Valdi::Value();
        }

        auto buffer = typedArray->getBuffer();
        auto bitmapSize = static_cast<size_t>(bitmapInfo.height * bitmapInfo.rowBytes);

        if (buffer.size() < bitmapSize) {
            callContext.getExceptionTracker().onError("Out of bounds computed bitmapSize vs provided buffer");
            return Valdi::Value();
        }

        return Valdi::Value(Valdi::makeShared<Valdi::BitmapWithBuffer>(buffer, bitmapInfo));
    }
}

Valdi::Value BitmapNativeModuleFactory::decodeNativeBitmap(const Valdi::ValueFunctionCallContext& callContext) {
    auto data = callContext.getParameter(0);
    Valdi::BytesView resolvedData;

    if (data.isString()) {
        auto bytes = Valdi::makeShared<Valdi::Bytes>();

        if (data.isStaticString()) {
            auto utf8Storage = data.getStaticString()->utf8Storage();
            if (!snap::utils::encoding::base64ToBinary(utf8Storage.toStringView(), *bytes)) {
                callContext.getExceptionTracker().onError("Failed to decode");
                return Valdi::Value();
            }
        } else {
            if (!snap::utils::encoding::base64ToBinary(data.toStringBox().toStringView(), *bytes)) {
                callContext.getExceptionTracker().onError("Failed to decode");
                return Valdi::Value();
            }
        }

        resolvedData = Valdi::BytesView(bytes);
    } else {
        auto typedArray = data.checkedTo<Valdi::Ref<Valdi::ValueTypedArray>>(callContext.getExceptionTracker());
        if (!callContext.getExceptionTracker()) {
            return Valdi::Value();
        }
        resolvedData = typedArray->getBuffer();
    }

    auto result = snap::drawing::Image::make(resolvedData);
    if (!result) {
        callContext.getExceptionTracker().onError(result.error());
        return Valdi::Value();
    }

    return Valdi::Value(result.value()->getBitmap());
}

static Ref<Valdi::IBitmap> getBitmapFromCallContext(const Valdi::ValueFunctionCallContext& callContext) {
    return callContext.getParameter(0).checkedToValdiObject<Valdi::IBitmap>(callContext.getExceptionTracker());
}

Valdi::Value BitmapNativeModuleFactory::disposeNativeBitmap(const Valdi::ValueFunctionCallContext& callContext) {
    auto bitmap = getBitmapFromCallContext(callContext);
    if (bitmap != nullptr) {
        bitmap->dispose();
    }

    return Valdi::Value();
}

Valdi::Value BitmapNativeModuleFactory::encodeNativeBitmap(const Valdi::ValueFunctionCallContext& callContext) {
    auto bitmap = getBitmapFromCallContext(callContext);
    if (bitmap == nullptr) {
        return Valdi::Value();
    }

    auto imageResult = Image::makeFromBitmap(bitmap, false);
    if (!imageResult) {
        callContext.getExceptionTracker().onError(imageResult.moveError());
        return Valdi::Value();
    }

    EncodedImageFormat format;
    auto formatIndex = callContext.getParameterAsInt(1);
    switch (formatIndex) {
        case 0:
            format = EncodedImageFormatJPG;
            break;
        case 1:
            format = EncodedImageFormatPNG;
            break;
        case 2:
            format = EncodedImageFormatWebP;
            break;
        default:
            callContext.getExceptionTracker().onError("Invalid encoding");
            return Valdi::Value();
    }

    auto qualityRatio = callContext.getParameterAsDouble(2);

    auto encoded = imageResult.value()->encode(format, qualityRatio);

    if (!encoded) {
        callContext.getExceptionTracker().onError(encoded.moveError());
        return Valdi::Value();
    }

    auto typedArray = Valdi::makeShared<Valdi::ValueTypedArray>(Valdi::ArrayBuffer, encoded.value());
    return Valdi::Value(typedArray);
}

Valdi::Value BitmapNativeModuleFactory::lockNativeBitmap(const Valdi::ValueFunctionCallContext& callContext) {
    auto bitmap = getBitmapFromCallContext(callContext);
    if (bitmap == nullptr) {
        return Valdi::Value();
    }

    auto* ptr = bitmap->lockBytes();
    if (ptr == nullptr) {
        callContext.getExceptionTracker().onError("Failed to lock pixels");
        return Valdi::Value();
    }
    auto bitmapInfo = bitmap->getInfo();
    auto bitmapSize = static_cast<size_t>(bitmapInfo.height * bitmapInfo.rowBytes);
    auto typedArray = Valdi::makeShared<Valdi::ValueTypedArray>(
        Valdi::ArrayBuffer, Valdi::BytesView(nullptr, reinterpret_cast<const Valdi::Byte*>(ptr), bitmapSize));
    return Valdi::Value(typedArray);
}

Valdi::Value BitmapNativeModuleFactory::unlockNativeBitmap(const Valdi::ValueFunctionCallContext& callContext) {
    auto bitmap = getBitmapFromCallContext(callContext);
    if (bitmap != nullptr) {
        bitmap->unlockBytes();
    }

    return Valdi::Value();
}

Valdi::Value BitmapNativeModuleFactory::getNativeBitmapInfo(const Valdi::ValueFunctionCallContext& callContext) {
    auto bitmap = getBitmapFromCallContext(callContext);
    if (bitmap == nullptr) {
        return Valdi::Value();
    }

    auto bitmapInfo = bitmap->getInfo();

    return Valdi::Value()
        .setMapValue("width", Valdi::Value(bitmapInfo.width))
        .setMapValue("height", Valdi::Value(bitmapInfo.height))
        .setMapValue("colorType", Valdi::Value(static_cast<int32_t>(bitmapInfo.colorType)))
        .setMapValue("alphaType", Valdi::Value(static_cast<int32_t>(bitmapInfo.alphaType)))
        .setMapValue("rowBytes", Valdi::Value(static_cast<int32_t>(bitmapInfo.rowBytes)));
}

} // namespace snap::drawing
