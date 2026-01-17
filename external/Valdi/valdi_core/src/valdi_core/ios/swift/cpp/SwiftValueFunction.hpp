//
//  SwiftValueFunction.h
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//
#pragma once
#include <valdi_core/cpp/Utils/ValueFunction.hpp>
#include <valdi_core/ios/swift/cpp/SwiftUtils.hpp>
#include <valdi_core/ios/swift/cpp/SwiftValdiMarshaller.h>

class SwiftValueFunction : public Valdi::ValueFunction {
public:
    // This constructor takes ownership of the SwiftValdiFunction
    SwiftValueFunction(SwiftValdiFunction function) : _swiftValdiFunction(function) {
        retainSwiftObject(_swiftValdiFunction);
    }

    ~SwiftValueFunction() {
        releaseSwiftObject(_swiftValdiFunction);
    }

    Valdi::Value operator()(const Valdi::ValueFunctionCallContext& callContext) noexcept final {
        Valdi::Marshaller marshaller(callContext.getExceptionTracker());
        int numParams = callContext.getParametersSize();
        for (int i = 0; i < numParams; i++) {
            marshaller.push(callContext.getParameter(i));
        }

        bool success = swiftCallWithMarshaller(_swiftValdiFunction, &marshaller);

        // Add a generic error to the exception tracker if no error was added
        if (!success && marshaller.getExceptionTracker()) {
            marshaller.getExceptionTracker().onError("Swift function call failed. Unknown error.");
        }

        bool isResultAvailable = marshaller.size() > numParams && success;
        if (!isResultAvailable) {
            return Valdi::Value::undefined();
        }
        return marshaller.get(-1);
    }

    std::string_view getFunctionType() const final {
        return "SwiftFunction";
    }

private:
    SwiftValdiFunction _swiftValdiFunction;
};