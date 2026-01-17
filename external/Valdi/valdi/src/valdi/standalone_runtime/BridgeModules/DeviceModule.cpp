//
//  DeviceModule.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/28/20.
//

#include "valdi/standalone_runtime/BridgeModules/DeviceModule.hpp"
#include "valdi_core/cpp/Localization/LocaleResolver.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueArray.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace Valdi {

#define BIND_METHOD(module, __method__)                                                                                \
    module.setMapValue(                                                                                                \
        STRINGIFY(__method__),                                                                                         \
        Valdi::Value(Valdi::ValueFunctionWithCallable::makeFromMethod(this, &DeviceModule::__method__)))

DeviceModule::DeviceModule() = default;

DeviceModule::~DeviceModule() = default;

void DeviceModule::setDisplaySize(double width, double height, double scale) {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    _displayWidth = width;
    _displayHeight = height;
    _displayScale = scale;
}

void DeviceModule::setWindowSize(double windowWidth, double windowHeight) {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    _windowWidth = windowWidth;
    _windowHeight = windowHeight;
}

Valdi::StringBox DeviceModule::getModulePath() {
    return STRING_LITERAL("valdi_core/src/DeviceBridge");
}

Valdi::Value DeviceModule::loadModule() {
    Valdi::Value module;

    BIND_METHOD(module, getDisplayWidth);
    BIND_METHOD(module, getDisplayHeight);
    BIND_METHOD(module, getWindowWidth);
    BIND_METHOD(module, getWindowHeight);
    BIND_METHOD(module, getDisplayScale);
    BIND_METHOD(module, getDisplayLeftInset);
    BIND_METHOD(module, getDisplayRightInset);
    BIND_METHOD(module, getDisplayBottomInset);
    BIND_METHOD(module, getDisplayTopInset);
    BIND_METHOD(module, getSystemType);
    BIND_METHOD(module, getDeviceLocales);

    BIND_METHOD(module, observeDarkMode);
    BIND_METHOD(module, observeDisplayInsetChange);

    BIND_METHOD(module, performHapticFeedback);

    BIND_METHOD(module, isDesktop);

    return module;
}

double DeviceModule::getDisplayWidth() {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    return _displayWidth;
}

double DeviceModule::getDisplayHeight() {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    return _displayHeight;
}

double DeviceModule::getWindowWidth() {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    return _windowWidth;
}

double DeviceModule::getWindowHeight() {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    return _windowHeight;
}

double DeviceModule::getDisplayScale() {
    std::lock_guard<Valdi::Mutex> guard(_mutex);
    return _displayScale;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
double DeviceModule::getDisplayLeftInset() {
    return 0;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
double DeviceModule::getDisplayRightInset() {
    return 0;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
double DeviceModule::getDisplayBottomInset() {
    return 0;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
double DeviceModule::getDisplayTopInset() {
    return 0;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::StringBox DeviceModule::getSystemType() {
    return STRING_LITERAL("ios");
}

Value DeviceModule::getDeviceLocales() {
    auto locales = LocaleResolver::getPreferredLocales();
    auto output = ValueArray::make(locales.size());

    size_t index = 0;
    for (const auto& locale : locales) {
        output->emplace(index++, Value(locale.trimmed()));
    }

    return Value(output);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool DeviceModule::isDesktop() {
    return true;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value DeviceModule::observeDarkMode() {
    Valdi::Value out;
    out.setMapValue(
        "cancel",
        Valdi::Value(Valdi::makeShared<Valdi::ValueFunctionWithCallable>(
            [](const Valdi::ValueFunctionCallContext& /*callContext*/) -> Valdi::Value { return Valdi::Value(); })));
    return out;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value DeviceModule::observeDisplayInsetChange() {
    Valdi::Value out;
    out.setMapValue(
        "cancel",
        Valdi::Value(Valdi::makeShared<Valdi::ValueFunctionWithCallable>(
            [](const Valdi::ValueFunctionCallContext& /*callContext*/) -> Valdi::Value { return Valdi::Value(); })));
    return out;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Valdi::Value DeviceModule::performHapticFeedback() {
    return Valdi::Value();
}

// Missing:
//@"getSystemVersion" : BRIDGE_METHOD(systemVersion),
//@"getLocaleUsesMetricSystem" : BRIDGE_METHOD(localeUsesMetricSystem),
//@"getTimeZoneRawSecondsFromGMT" : BRIDGE_METHOD(timeZoneRawSecondsFromGMT),
//@"getTimeZoneDstSecondsFromGMT" : BRIDGE_METHOD(timeZoneDstSecondsFromGMT),

} // namespace Valdi
