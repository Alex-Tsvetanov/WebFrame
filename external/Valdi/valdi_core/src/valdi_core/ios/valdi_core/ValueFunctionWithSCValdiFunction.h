//
//  ValueFunctionWithSCValdiFunction.h
//  valdi-ios
//
//  Created by Simon Corsin on 5/19/21.
//

#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/cpp/Utils/ValueFunction.hpp"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

namespace ValdiIOS {

/**
 A ValueFunction backed an Objective-C SCValdiFunction instance.
 */
class ValueFunctionWithSCValdiFunction : public Valdi::ValueFunction, public ValdiIOS::ObjCObjectIndirectRef {
public:
    explicit ValueFunctionWithSCValdiFunction(__unsafe_unretained id<SCValdiFunction> function);
    ~ValueFunctionWithSCValdiFunction() override;

    virtual Valdi::Value operator()(const Valdi::ValueFunctionCallContext& callContext) noexcept override;

    std::string_view getFunctionType() const override;

    id<SCValdiFunction> getFunction() const;
};

} // namespace ValdiIOS

NS_ASSUME_NONNULL_END
