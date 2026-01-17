//
//  ValdiObject.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/10/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#define VALDI_CLASS_NAME_BODY_IMPL(clsName)                                                                            \
    {                                                                                                                  \
        static auto kClsName = STRING_LITERAL(STRINGIFY(clsName));                                                     \
        return kClsName;                                                                                               \
    }

// for direct impl in headers
#define VALDI_CLASS_HEADER_IMPL(clsName)                                                                               \
    const Valdi::StringBox& getClassName() const override VALDI_CLASS_NAME_BODY_IMPL(clsName)

// for header/impl
#define VALDI_CLASS_HEADER(clsName) const Valdi::StringBox& getClassName() const override;
#define VALDI_CLASS_IMPL(clsName)                                                                                      \
    const Valdi::StringBox& clsName::getClassName() const VALDI_CLASS_NAME_BODY_IMPL(clsName)

namespace Valdi {

class ValdiObject : public SharedPtrRefCountable {
public:
    ~ValdiObject() override;

    virtual const StringBox& getClassName() const = 0;

    virtual StringBox getDebugDescription() const;

    StringBox getDebugId() const;
};

using SharedValdiObject = Ref<ValdiObject>;

} // namespace Valdi
