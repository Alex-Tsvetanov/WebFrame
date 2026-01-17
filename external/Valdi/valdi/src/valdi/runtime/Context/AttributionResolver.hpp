// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "valdi_core/cpp/Context/Attribution.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class ILogger;

class AttributionResolver : public SimpleRefCountable {
public:
    struct MappingEntry {
        const char* moduleName;
        const char* owner;
        AttributionFunction callback;
    };

    AttributionResolver(const MappingEntry* mapping, size_t length, ILogger& logger);
    ~AttributionResolver() override;

    Attribution resolveAttribution(const StringBox& moduleName);

    static Attribution getDefaultAttribution();

private:
    struct OwnerWithCallback {
        StringBox owner;
        AttributionFunction callback = nullptr;
    };
    FlatMap<StringBox, OwnerWithCallback> _mapping;
    [[maybe_unused]] ILogger& _logger;
};

#define VALDI_ATTRIBUTION_FN_NAME(owner, moduleName)                                                                   \
    __VALDI_IS_CALLING_OUT_TO_CODE_OWNED_BY__##owner##__IN_MODULE__##moduleName
#define VALDI_ATTRIBUTE_MODULE_NAME_STR(moduleName) #moduleName

#define VALDI_ATTRIBUTION_FN(owner, moduleName)                                                                        \
    __attribute__((noinline, used, disable_tail_calls)) const void* VALDI_ATTRIBUTION_FN_NAME(owner, moduleName)(      \
        const Valdi::AttributionFunctionCallback& fn) {                                                                \
        fn();                                                                                                          \
        return reinterpret_cast<const void*>(VALDI_ATTRIBUTE_MODULE_NAME_STR(moduleName));                             \
    }

} // namespace Valdi
