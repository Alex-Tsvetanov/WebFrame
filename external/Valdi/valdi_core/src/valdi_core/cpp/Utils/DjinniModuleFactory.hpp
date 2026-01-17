#pragma once

#include "valdi_core/ModuleFactory.hpp"

namespace Valdi {

template<typename T, typename... E>
class DjinniModuleFactory : public snap::valdi_core::ModuleFactory, public std::enable_shared_from_this<T> {
    template<typename U0, typename... U>
    struct EntryPoint {
        static void doRegister(Ref<ValueMap> m) {
            U0::djinniInitStaticMethods(m);
            EntryPoint<U...>::doRegister(m);
        }
    };
    template<typename U0>
    struct EntryPoint<U0> {
        static void doRegister(Ref<ValueMap> m) {
            U0::djinniInitStaticMethods(m);
        }
    };

public:
    explicit DjinniModuleFactory(const StringBox& name) : _name(name) {}

    StringBox getModulePath() override {
        return _name;
    }

    Value loadModule() override {
        // module object
        auto m = makeShared<ValueMap>();
        EntryPoint<E...>::doRegister(m);
        return Value(m);
    }

private:
    const StringBox _name;
};

} // namespace Valdi
