#include "TSNTestUtils.hpp"
#include "tsn/tsn.h"
#include <string_view>

namespace ValdiTest {

static tsn_value sayHello(tsn_vm* ctx,
                          tsn_value_const this_val,
                          tsn_stackframe* /*stackframe*/,
                          int argc,
                          tsn_value_const* argv,
                          tsn_closure* closure) {
    std::string_view str = "Hello from Native!";
    return tsn_new_string_with_len(ctx, str.data(), str.size());
}

static tsn_value doLoadModule(tsn_vm* ctx,
                              tsn_value_const this_val,
                              tsn_stackframe* /*stackframe*/,
                              int argc,
                              tsn_value_const* argv,
                              tsn_closure* closure) {
    auto functionName = closure->module->atoms[0];
    auto helloFn = tsn_new_function(ctx, closure->module, functionName, &sayHello, 0);
    if (tsn_is_exception(ctx, helloFn)) {
        return helloFn;
    }

    auto exports = tsn_get_func_arg(ctx, argc, argv, 2);
    auto ret = tsn_set_property(ctx, &exports, functionName, &helloFn);
    tsn_release(ctx, helloFn);
    tsn_release(ctx, exports);

    if (ret < 0) {
        return tsn_exception(ctx);
    } else {
        return tsn_undefined(ctx);
    }
}

static tsn_value moduleInitFn(tsn_vm* ctx) {
    auto* tsnModule = tsn_new_module(ctx, 1, 0, 1);
    if (tsnModule == nullptr) {
        return tsn_exception(ctx);
    }

    tsnModule->atoms[0] = tsn_create_atom_len(ctx, "hello", 5);

    auto moduleLoadFn = tsn_new_arrow_function_closure(ctx, tsnModule, &doLoadModule, 0, 0, nullptr, 0, 0, 0);

    tsn_free_module(ctx, tsnModule);

    return moduleLoadFn;
}

void registerTSNTestModule(const char* moduleName) {
    tsn_register_module_post_launch(moduleName, "", &moduleInitFn);
}

} // namespace ValdiTest
