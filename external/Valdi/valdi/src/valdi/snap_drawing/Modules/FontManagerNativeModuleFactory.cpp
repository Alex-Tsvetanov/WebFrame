//
//  FontManagerNativeModuleFactory.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 5/16/24.
//

#include "valdi/snap_drawing/Modules/FontManagerNativeModuleFactory.hpp"
#include "snap_drawing/cpp/Resources.hpp"
#include "snap_drawing/cpp/Text/FontManager.hpp"
#include "valdi/snap_drawing/Runtime.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithMethod.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace snap::drawing {

FontManagerNativeModuleFactory::FontManagerNativeModuleFactory(Valdi::Function<Ref<Runtime>()> runtimeProvider)
    : _runtimeProvider(std::move(runtimeProvider)) {}

FontManagerNativeModuleFactory::~FontManagerNativeModuleFactory() = default;

Valdi::StringBox FontManagerNativeModuleFactory::getModulePath() {
    return STRING_LITERAL("drawing/src/FontManagerNative");
}

Valdi::Value FontManagerNativeModuleFactory::loadModule() {
    Valdi::Value out;

    Valdi::ValueFunctionMethodBinder<FontManagerNativeModuleFactory> binder(this, out);
    binder.bind("getDefaultFontManager", &FontManagerNativeModuleFactory::getDefaultFontManager);
    binder.bind("makeScopedFontManager", &FontManagerNativeModuleFactory::makeScopedFontManager);
    binder.bind("registerFontFromData", &FontManagerNativeModuleFactory::registerFontFromData);
    binder.bind("registerFontFromFilePath", &FontManagerNativeModuleFactory::registerFontFromFilePath);

    return out;
}

Valdi::Value FontManagerNativeModuleFactory::getDefaultFontManager(const Valdi::ValueFunctionCallContext& callContext) {
    return Valdi::Value(_runtimeProvider()->getResources()->getFontManager());
}

static Valdi::Ref<IFontManager> getFontManagerFromCallContext(const Valdi::ValueFunctionCallContext& callContext) {
    return callContext.getParameter(0).checkedToValdiObject<IFontManager>(callContext.getExceptionTracker());
}

Valdi::Value FontManagerNativeModuleFactory::makeScopedFontManager(const Valdi::ValueFunctionCallContext& callContext) {
    auto fontManager = getFontManagerFromCallContext(callContext);
    if (fontManager == nullptr) {
        return Valdi::Value::undefined();
    }

    return Valdi::Value(fontManager->makeScoped());
}

static Valdi::Value doRegisterFont(const Valdi::ValueFunctionCallContext& callContext,
                                   const Valdi::StringBox& fontName,
                                   const Ref<snap::drawing::LoadableTypeface>& loadableTypeface) {
    auto fontManager = getFontManagerFromCallContext(callContext);
    if (fontManager != nullptr) {
        auto fontWeightStr = callContext.getParameter(2).checkedTo<Valdi::StringBox>(callContext.getExceptionTracker());
        if (!callContext.getExceptionTracker()) {
            return Valdi::Value();
        }
        auto fontStyleStr = callContext.getParameter(3).checkedTo<Valdi::StringBox>(callContext.getExceptionTracker());
        if (!callContext.getExceptionTracker()) {
            return Valdi::Value();
        }

        auto fontWeight = FontStyle::parseWeight(fontWeightStr.toStringView());
        if (!fontWeight) {
            callContext.getExceptionTracker().onError(fontWeight.moveError());
            return Valdi::Value();
        }
        auto fontSlant = FontStyle::parseSlant(fontStyleStr.toStringView());
        if (!fontSlant) {
            callContext.getExceptionTracker().onError(fontWeight.moveError());
            return Valdi::Value();
        }

        fontManager->registerTypeface(
            fontName, FontStyle(FontWidthNormal, fontWeight.value(), fontSlant.value()), false, loadableTypeface);
    }

    return Valdi::Value::undefined();
}

Valdi::Value FontManagerNativeModuleFactory::registerFontFromData(const Valdi::ValueFunctionCallContext& callContext) {
    auto fontData =
        callContext.getParameter(4).checkedTo<Ref<Valdi::ValueTypedArray>>(callContext.getExceptionTracker());
    if (fontData == nullptr) {
        return Valdi::Value();
    }

    auto fontName = callContext.getParameter(1).checkedTo<Valdi::StringBox>(callContext.getExceptionTracker());
    if (!callContext.getExceptionTracker()) {
        return Valdi::Value();
    }

    return doRegisterFont(callContext, fontName, LoadableTypeface::fromBytes(fontName, fontData->getBuffer()));
}

Valdi::Value FontManagerNativeModuleFactory::registerFontFromFilePath(
    const Valdi::ValueFunctionCallContext& callContext) {
    auto fontPath = callContext.getParameter(4).checkedTo<Valdi::StringBox>(callContext.getExceptionTracker());
    if (fontPath == nullptr) {
        return Valdi::Value();
    }

    auto fontName = callContext.getParameter(1).checkedTo<Valdi::StringBox>(callContext.getExceptionTracker());
    if (!callContext.getExceptionTracker()) {
        return Valdi::Value();
    }

    return doRegisterFont(callContext, fontName, LoadableTypeface::fromFile(fontName, fontName));
}

} // namespace snap::drawing
