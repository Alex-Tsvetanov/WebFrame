//
//  DeviceModule.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/28/20.
//

#pragma once

#include "valdi_core/ModuleFactory.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

class DeviceModule : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    DeviceModule();
    ~DeviceModule() override;

    Valdi::StringBox getModulePath() override;
    Valdi::Value loadModule() override;

    void setDisplaySize(double width, double height, double scale);
    void setWindowSize(double windowWidth, double windowHeight);

    double getDisplayWidth();
    double getDisplayHeight();
    double getDisplayScale();

    double getWindowWidth();
    double getWindowHeight();

    double getDisplayLeftInset();
    double getDisplayRightInset();
    double getDisplayBottomInset();
    double getDisplayTopInset();

    Valdi::StringBox getSystemType();

    Valdi::Value getDeviceLocales();

    Valdi::Value performHapticFeedback();

    bool isDesktop();

    Valdi::Value observeDarkMode();
    Valdi::Value observeDisplayInsetChange();

    Valdi::Value writeFile(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value readFileData(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value readFileString(const Valdi::ValueFunctionCallContext& callContext);
    Valdi::Value readDirectory(const Valdi::ValueFunctionCallContext& callContext);

private:
    mutable Valdi::Mutex _mutex;

    double _displayWidth = 0.0;
    double _displayHeight = 0.0;
    double _displayScale = 1.0;

    double _windowWidth = 0.0;
    double _windowHeight = 0.0;
};

} // namespace Valdi
