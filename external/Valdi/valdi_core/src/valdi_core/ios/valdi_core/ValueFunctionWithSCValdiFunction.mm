//
//  ValueFunctionWithSCValdiFunction.m
//  valdi-ios
//
//  Created by Simon Corsin on 5/19/21.
//

#import "valdi_core/ValueFunctionWithSCValdiFunction.h"
#import "valdi_core/SCValdiMarshaller+CPP.h"
#import "valdi_core/SCValdiError.h"
#import "valdi_core/SCValdiValueUtils.h"

namespace ValdiIOS {

ValueFunctionWithSCValdiFunction::ValueFunctionWithSCValdiFunction(__unsafe_unretained id<SCValdiFunction> function): ValdiIOS::ObjCObjectIndirectRef(function, YES) {

}

ValueFunctionWithSCValdiFunction::~ValueFunctionWithSCValdiFunction() = default;

Valdi::Value ValueFunctionWithSCValdiFunction::operator()(const Valdi::ValueFunctionCallContext &callContext) noexcept {

    @try {
        Valdi::Marshaller stack(callContext.getExceptionTracker());
        for (size_t i = 0; i < callContext.getParametersSize(); i++) {
            stack.push(callContext.getParameter(i));
        }

        SCValdiMarshallerRef marshaller = SCValdiMarshallerWrap(&stack);
        if ([getFunction() performWithMarshaller:marshaller]) {
            return stack.getOrUndefined(-1);
        }

        return Valdi::Value::undefined();

    } @catch (SCValdiError *error) {
        callContext.getExceptionTracker().onError(ErrorFromNSException(error));

        return Valdi::Value::undefined();
    }
}

std::string_view ValueFunctionWithSCValdiFunction::getFunctionType() const {
    return "ObjC";
}

id<SCValdiFunction> ValueFunctionWithSCValdiFunction::getFunction() const {
    return getValue();
}

}



