#include "tsn/tsn.h"
#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "valdi_core/cpp/Attributes/AttributeUtils.hpp"
#include "valdi_core/cpp/Threading/Thread.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

// NOLINTBEGIN(readability-identifier-naming)

extern "C" const JSAtom JS_ATOM_value_tsn;
extern "C" const JSAtom JS_ATOM_done_tsn;
extern "C" const JSAtom JS_ATOM_next_tsn;
extern "C" const JSAtom JS_ATOM_return_tsn;
extern "C" const JSAtom JS_ATOM_throw_tsn;
extern "C" const JSAtom JS_ATOM_Symbol_iterator_tsn;

namespace tsn {

class ValueRef {
public:
    ValueRef(tsn_vm* ctx, tsn_value value) : _ctx(ctx), _value(value) {}

    ~ValueRef() {
        JS_FreeValue(_ctx, _value);
    }

    ValueRef& operator=(tsn_value value) {
        JS_FreeValue(_ctx, _value);
        _value = value;

        return *this;
    }

    inline tsn_vm* context() const {
        return _ctx;
    }

    inline const tsn_value& operator*() const {
        return _value;
    }

    inline const tsn_value& get() const {
        return _value;
    }

    inline tsn_value& get() {
        return _value;
    }

    tsn_value release() {
        auto value = _value;
        _value = JS_UNDEFINED;
        return value;
    }

private:
    tsn_vm* _ctx;
    tsn_value _value;
};

class ModuleRegistry {
public:
    ModuleRegistry() = default;

    struct ModuleInfo {
        const char* name = nullptr;
        const char* sha256 = nullptr;
        tsn_module_init_fn moduleInitFn = nullptr;
    };

    void unsafeRegisterModule(const char* moduleName, const char* sha256, tsn_module_init_fn moduleInitFn) {
        auto& it = _pendingModules.emplace_back();
        it.moduleName = moduleName;
        it.sha256 = sha256;
        it.moduleInitFn = moduleInitFn;
    }

    void registerModule(const char* moduleName, const char* sha256, tsn_module_init_fn moduleInfo) {
        std::lock_guard<Valdi::Mutex> lock(_mutex);
        unsafeRegisterModule(moduleName, sha256, moduleInfo);
    }

    std::optional<ModuleInfo> getModuleInfo(std::string_view moduleName) {
        std::lock_guard<Valdi::Mutex> lock(_mutex);
        if (!_pendingModules.empty()) {
            _registeredModules.reserve(_registeredModules.size() + _pendingModules.size());

            for (const auto& it : _pendingModules) {
                auto& inserted = _registeredModules[std::string_view(it.moduleName)];
                inserted.name = it.moduleName;
                inserted.sha256 = it.sha256;
                inserted.moduleInitFn = it.moduleInitFn;
            }
        }

        const auto& it = _registeredModules.find(moduleName);
        if (it == _registeredModules.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    static ModuleRegistry& getInstance() {
        static auto* kInstance = new ModuleRegistry();

        return *kInstance;
    }

private:
    struct PendingModule {
        const char* moduleName = nullptr;
        const char* sha256 = nullptr;
        tsn_module_init_fn moduleInitFn = nullptr;
    };

    Valdi::Mutex _mutex;
    std::vector<PendingModule> _pendingModules;
    Valdi::FlatMap<std::string_view, ModuleInfo> _registeredModules;
};

} // namespace tsn

#define TSN_CHECK_EXCEPTION(value_ref)                                                                                 \
    if (tsn_is_exception((value_ref).context(), (value_ref).get())) {                                                  \
        return tsn_exception((value_ref).context());                                                                   \
    }

using namespace std;
using namespace tsn;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// NOLINTNEXTLINE(modernize-use-using)
typedef struct {
    JSValue function_prototype;
    JSAtom home_object_atom;
    const char* current_module_name;
} tsn_vm_helpers;

JSClassID newClassID() {
    static auto* kMutex = new std::mutex();

    auto lockGuard = std::lock_guard<std::mutex>(*kMutex);

    JSClassID classID = 0;
    return JS_NewClassID(&classID);
}

static JSClassID getTSNFunctionClassID() {
    static auto kFunctionClassID = newClassID();
    return kFunctionClassID;
}

static JSClassID getTSNModuleClassID() {
    static auto kModuleClassID = newClassID();
    return kModuleClassID;
}

static inline tsn_vm_helpers* tsn_get_vm_helpers(tsn_vm* ctx) {
    return reinterpret_cast<tsn_vm_helpers*>(JS_GetContextOpaque(ctx));
}

template<typename F>
static inline tsn_result withNameAsAtom(tsn_vm* ctx, tsn_value name, F&& cb) {
    auto atomName = JS_ValueToAtom(ctx, name);
    if (atomName == JS_ATOM_NULL) {
        return tsn_result_failure;
    }

    auto result = cb(atomName);
    tsn_free_atom(ctx, atomName);

    return result;
}

template<typename F>
static inline tsn_value withNameAsAtomValueRet(tsn_vm* ctx, tsn_value name, F&& cb) {
    auto atomName = JS_ValueToAtom(ctx, name);
    if (atomName == JS_ATOM_NULL) {
        return tsn_exception(ctx);
    }

    auto result = cb(atomName);
    tsn_free_atom(ctx, atomName);

    return result;
}

js_force_inline tsn_result tsn_process_result(tsn_vm* /*ctx*/, int result) {
    if (result < 0) {
        return tsn_result_failure;
    } else {
        return tsn_result_success;
    }
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static tsn_value tsn_print(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    int i;
    const char* str;
    size_t len;

    auto global = JS_GetGlobalObject(ctx);
    auto json = JS_GetPropertyStr(ctx, global, "JSON");
    auto stringify = JS_GetPropertyStr(ctx, json, "stringify");

    for (i = 0; i < argc; i++) {
        JSValue printVal;
        JSValue arg = argv[i];
        if (JS_IsObject(arg) != 0 && JS_IsFunction(ctx, arg) == 0) {
            printVal = JS_CallInternal_tsn(ctx, stringify, json, JS_UNDEFINED, 1, (tsn_value[]){arg}, 0);
        } else {
            printVal = JS_ToString(ctx, arg);
        }

        if (i != 0)
            putchar(' ');
        str = JS_ToCStringLen(ctx, &len, printVal);
        if (!str)
            return JS_EXCEPTION;
        fwrite(str, 1, len, stdout);
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, printVal);
    }
    putchar('\n');

    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, json);
    JS_FreeValue(ctx, stringify);

    return tsn_undefined(ctx);
}

static tsn_value tsn_require_absolute(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    if (argc < 1) {
        return JS_ThrowReferenceError(ctx, "Missing module name argument");
    }

    auto moduleName = JS_ValueToAtom(ctx, argv[0]);

    auto loadedModule = tsn_require(ctx, moduleName);

    tsn_free_atom(ctx, moduleName);

    return loadedModule;
}

static tsn_value tsn_require_relative(
    tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv, int /*magic*/, tsn_value* func_data) {
    if (argc < 1) {
        return JS_ThrowReferenceError(ctx, "Missing module name argument");
    }

    auto parentModulePath = ValueRef(ctx, JS_GetPropertyStr(ctx, func_data[0], "path"));
    TSN_CHECK_EXCEPTION(parentModulePath);

    auto parentModulePathAtom = JS_ValueToAtom(ctx, *parentModulePath);
    if (parentModulePathAtom == 0) {
        return tsn_exception(ctx);
    }

    auto requestedModulePath = JS_ValueToAtom(ctx, argv[0]);
    if (requestedModulePath == 0) {
        JS_FreeAtom(ctx, parentModulePathAtom);
        return tsn_exception(ctx);
    }

    auto* normalizedName = JS_NormalizeModuleName(
        ctx, JS_AtomToCString(ctx, parentModulePathAtom), JS_AtomToCString(ctx, requestedModulePath));
    JS_FreeAtom(ctx, requestedModulePath);
    JS_FreeAtom(ctx, parentModulePathAtom);

    if (normalizedName == nullptr) {
        return tsn_exception(ctx);
    }

    auto output = tsn_require_from_string(ctx, normalizedName);
    js_free(ctx, normalizedName);
    return output;
}

tsn_value tsn_get_property0(tsn_vm* ctx, const tsn_value_const* obj, tsn_atom prop) {
    return JS_GetProperty(ctx, *obj, prop);
}

tsn_value tsn_require(tsn_vm* ctx, tsn_atom module_name) {
    auto global = ValueRef(ctx, JS_GetGlobalObject(ctx));
    TSN_CHECK_EXCEPTION(global);

    auto modules = ValueRef(ctx, JS_GetPropertyStr(ctx, *global, "__modules__"));
    TSN_CHECK_EXCEPTION(modules);

    if (tsn_is_undefined(ctx, *modules)) {
        modules = JS_NewObject(ctx);
        TSN_CHECK_EXCEPTION(modules);

        if (tsn_set_property_str(ctx, *global, "__modules__", *modules) < 0) {
            return tsn_exception(ctx);
        }
    }

    // don't use prop cache in the GetProperty call because module_name is not fixed
    auto alreadyLoadedModule = ValueRef(ctx, JS_GetProperty(ctx, *modules, module_name));
    if (!tsn_is_undefined(ctx, *alreadyLoadedModule)) {
        return alreadyLoadedModule.release();
    }

    auto moduleInitFunc = ValueRef(ctx, tsn_load_module(ctx, JS_AtomToCString(ctx, module_name)));
    TSN_CHECK_EXCEPTION(moduleInitFunc);

    auto moduleVar = ValueRef(ctx, JS_NewObject(ctx));
    TSN_CHECK_EXCEPTION(moduleVar);
    auto exportsVar = ValueRef(ctx, JS_NewObject(ctx));
    TSN_CHECK_EXCEPTION(exportsVar);
    auto requireFunc = ValueRef(ctx, JS_NewCFunctionData(ctx, &tsn_require_relative, 0, 0, 1, &moduleVar.get()));
    TSN_CHECK_EXCEPTION(exportsVar);

    auto pathVar = ValueRef(ctx, JS_AtomToValue(ctx, module_name));
    TSN_CHECK_EXCEPTION(exportsVar);

    if (tsn_set_property_str(ctx, *moduleVar, "path", *pathVar) < 0) {
        return tsn_exception(ctx);
    }

    if (tsn_set_property_str(ctx, *moduleVar, "exports", *exportsVar) < 0) {
        return tsn_exception(ctx);
    }

    if (tsn_set_property(ctx, &modules.get(), module_name, &exportsVar.get()) < 0) {
        return tsn_exception(ctx);
    }

    // Now load the module

    auto loadedModule = ValueRef(ctx,
                                 JS_CallInternal_tsn(ctx,
                                                     *moduleInitFunc,
                                                     tsn_undefined(ctx),
                                                     tsn_undefined(ctx),
                                                     3,
                                                     (tsn_value[]){*requireFunc, *moduleVar, *exportsVar},
                                                     0));
    if (tsn_is_exception(ctx, *loadedModule)) {
        auto exception = ValueRef(ctx, JS_GetException(ctx));
        // Clear exports from __modules__
        auto undefined = tsn_undefined(ctx);
        if (tsn_set_property(ctx, &modules.get(), module_name, &undefined) < 0) {
            // Ignore exception when clearing
            JS_FreeValue(ctx, JS_GetException(ctx));
        }

        tsn_throw(ctx, *exception);
        return tsn_exception(ctx);
    }

    return loadedModule.release();
}

tsn_value tsn_require_from_string(tsn_vm* ctx, const char* module_name) {
    auto moduleNameAtom = JS_NewAtom(ctx, module_name);
    if (moduleNameAtom == 0) {
        return tsn_exception(ctx);
    }

    auto output = tsn_require(ctx, moduleNameAtom);
    JS_FreeAtom(ctx, moduleNameAtom);
    return output;
}

void tsn_debug(tsn_vm* ctx, tsn_value_const value, const char* expression) {
    std::string script = "(($input) => ";
    script += std::string_view(expression);
    script += ")";

    auto fn = JS_Eval(ctx, script.data(), script.size(), "debug.json", 0);
    if (tsn_is_exception(ctx, fn)) {
        tsn_dump_error(ctx);
        return;
    }

    auto retValue = JS_CallInternal_tsn(ctx, fn, tsn_undefined(ctx), tsn_undefined(ctx), 1, (tsn_value[]){value}, 0);
    JS_FreeValue(ctx, fn);

    if (tsn_is_exception(ctx, retValue)) {
        tsn_dump_error(ctx);
        return;
    }

    auto print_result = tsn_print(ctx, tsn_undefined(ctx), 1, (tsn_value[]){retValue});
    if (tsn_is_exception(ctx, print_result)) {
        tsn_dump_error(ctx);
    }

    JS_FreeValue(ctx, retValue);
}

struct Timer {
    int64_t id;
    tsn_vm* ctx;
    tsn_value_const func;
    std::vector<tsn_value_const> args;

    void dispose() {
        JS_FreeValue(ctx, func);
        for (auto& arg : args) {
            JS_FreeValue(ctx, arg);
        }
    }
};
static int64_t next_timer_id = 0;
static std::multimap<std::chrono::steady_clock::time_point, Timer> active_timers;

// setTimeout(func, delay?, params?...)
//            0     1       2
static tsn_value tsn_set_timeout(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    std::vector<tsn_value_const> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(JS_DupValue(ctx, argv[i]));
    }
    auto deadline = std::chrono::steady_clock::now();
    if (argc > 1) { // has delay
        double ms = 0;
        if (JS_ToFloat64(ctx, &ms, argv[1]) != 0) {
            return tsn_exception(ctx); // not a number
        }
        // convert milli to microseconds for better precision
        deadline += std::chrono::microseconds(static_cast<int64_t>(ms * 1000.0));
    }
    auto tid = ++next_timer_id;
    active_timers.emplace(deadline,
                          Timer{
                              tid,                       // id
                              ctx,                       // ctx
                              JS_DupValue(ctx, argv[0]), // func
                              std::move(args)            // args
                          });
    return JS_NewInt64(ctx, tid);
}
// clearTimeout(timerId)
static tsn_value tsn_clear_timeout(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    int64_t tid = -1;
    if (JS_ToInt64(ctx, &tid, argv[0]) != 0) {
        return tsn_exception(ctx); // not a number
    }
    for (auto i = active_timers.begin(); i != active_timers.end(); ++i) {
        if (i->second.id == tid) {
            i->second.dispose();
            active_timers.erase(i);
            break;
        }
    }
    return tsn_undefined(ctx);
}

int32_t exitCode = 0;
static tsn_value tsn_set_exit_code(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    if (argc > 0) { // has code
        JS_ToInt32(ctx, &exitCode, argv[0]);
    }
    return JS_UNDEFINED;
}

// returns number of active times
int tsn_handle_timers() {
    auto now = std::chrono::steady_clock::now();
    auto expired = active_timers.upper_bound(now);

    // move all expired timers to separate container
    std::vector<Timer> expired_timers;
    for (auto i = active_timers.begin(); i != expired; ++i) {
        expired_timers.push_back(std::move(i->second));
    }
    if (!expired_timers.empty()) {
        active_timers.erase(active_timers.begin(), expired);
        // run all expired timers
        for (auto& timer : expired_timers) {
            auto res = tsn_call(timer.ctx, timer.func, JS_UNDEFINED, timer.args.size(), timer.args.data());
            tsn_release_inline(timer.ctx, res);
            timer.dispose();
        }
    }
    return active_timers.size();
}

auto tsn_next_timer() {
    if (active_timers.empty()) {
        return std::chrono::steady_clock::now();
    } else {
        return active_timers.begin()->first;
    }
}

static tsn_value tsn_write_file(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    const char* filename;
    const char* str;
    filename = JS_ToCString(ctx, argv[0]);
    str = JS_ToCString(ctx, argv[1]);
    std::ofstream f(filename);
    f << str;
    JS_FreeCString(ctx, filename);
    JS_FreeCString(ctx, str);
    return JS_DupValue(ctx, argv[0]);
}

static tsn_value tsn_read_file(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    const char* filename;
    filename = JS_ToCString(ctx, argv[0]);
    std::ifstream f(filename);
    std::string str((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    JS_FreeCString(ctx, filename);
    return JS_NewStringLen(ctx, str.data(), str.size());
    ;
}

static tsn_value tsn_now(tsn_vm* ctx, tsn_value_const /*this_val*/, int argc, tsn_value_const* argv) {
    auto nano_since_epoch = std::chrono::high_resolution_clock::now().time_since_epoch() / std::chrono::nanoseconds(1);
    return JS_NewFloat64(ctx, nano_since_epoch / 1000000.0);
}

void tsn_add_helpers(tsn_vm* ctx) {
    auto global = JS_GetGlobalObject(ctx);
    auto std = JS_NewObject(ctx);

    auto fs = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, fs, "writeFileSync", JS_NewCFunction(ctx, &tsn_write_file, "writeFileSync", 1));
    JS_SetPropertyStr(ctx, fs, "readFileSync", JS_NewCFunction(ctx, &tsn_read_file, "readFileSync", 1));
    JS_SetPropertyStr(ctx, global, "fs", fs);

    auto os = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, os, "now", JS_NewCFunction(ctx, &tsn_now, "now", 0));
    JS_SetPropertyStr(ctx, global, "os", os);

    JS_SetPropertyStr(ctx, std, "log", JS_NewCFunction(ctx, &tsn_print, "log", 1));
    JS_SetPropertyStr(ctx, std, "error", JS_NewCFunction(ctx, &tsn_print, "error", 1));
    JS_SetPropertyStr(ctx, global, "console", std);
    // Provides basic support for importing modules in a pure TSN environment
    JS_SetPropertyStr(ctx, global, "require", JS_NewCFunction(ctx, tsn_require_absolute, "require", 1));
    // Provider Basic timer for testing async code
    JS_SetPropertyStr(ctx, global, "setTimeout", JS_NewCFunction(ctx, tsn_set_timeout, "setTimeout", 1));
    JS_SetPropertyStr(ctx, global, "clearTimeout", JS_NewCFunction(ctx, tsn_clear_timeout, "clearTimeout", 1));

    // Async tests are all wrapped in promises this means we will never get a real exception in the main function.
    // So provide a backdoor to set the exit code of the test suite
    JS_SetPropertyStr(ctx, global, "setExitCode", JS_NewCFunction(ctx, tsn_set_exit_code, "setExitCode", 1));

    JS_SetPropertyStr(ctx, global, "global", JS_DupValue(ctx, global));
    JS_FreeValue(ctx, global);
}

void tsn_dump_obj(tsn_vm* ctx, FILE* f, tsn_value val) {
    const char* str;

    str = JS_ToCString(ctx, val);
    if (str) {
        fprintf(f, "%s\n", str);
        JS_FreeCString(ctx, str);
    } else {
        fprintf(f, "[exception]\n");
    }
}

static void tsn_dump_error1(tsn_vm* ctx, tsn_value_const exception_val) {
    tsn_value val;
    bool is_error;

    is_error = JS_IsError(ctx, exception_val) != 0;
    tsn_dump_obj(ctx, stderr, exception_val);
    if (is_error) {
        val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (JS_IsUndefined(val) == 0) {
            tsn_dump_obj(ctx, stderr, val);
        }
        JS_FreeValue(ctx, val);
    }
}

bool tsn_has_exception(tsn_vm* ctx) {
    return JS_HasException_tsn(ctx) != 0;
}

enum class tsn_binary_arith_slow_op {
    sub,
    mul,
    div,
    mod,
    pow,
};

static double tsn_pow(double a, double b) {
    if (unlikely(!isfinite(b)) && fabs(a) == 1) {
        /* not compatible with IEEE 754 */
        return JS_FLOAT64_NAN;
    } else {
        return pow(a, b);
    }
}

static inline tsn_value tsn_binary_arith_slow(tsn_vm* ctx,
                                              tsn_binary_arith_slow_op op,
                                              tsn_value_const op1,
                                              tsn_value_const op2) {
    double d1;
    double d2;
    double r;

    if (unlikely(JS_ToFloat64(ctx, &d1, op1) != 0)) {
        JS_FreeValue(ctx, op2);
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToFloat64(ctx, &d2, op2) != 0)) {
        return JS_EXCEPTION;
    }
    switch (op) {
        case tsn_binary_arith_slow_op::sub:
            r = d1 - d2;
            break;
        case tsn_binary_arith_slow_op::mul:
            r = d1 * d2;
            break;
        case tsn_binary_arith_slow_op::div:
            r = d1 / d2;
            break;
        case tsn_binary_arith_slow_op::mod:
            r = fmod(d1, d2);
            break;
        case tsn_binary_arith_slow_op::pow:
            r = tsn_pow(d1, d2);
            break;
        default:
            abort();
    }
    return JS_NewFloat64(ctx, r);
}

js_force_inline tsn_value tsn_add_slow_numbers(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    double d1;
    double d2;
    if (JS_ToFloat64(ctx, &d1, op1) != 0) {
        return JS_EXCEPTION;
    }
    if (JS_ToFloat64(ctx, &d2, op2) != 0) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, d1 + d2);
}

static inline tsn_value tsn_add_slow(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t tag1;
    uint32_t tag2;

    tag1 = JS_VALUE_GET_TAG(op1);
    tag2 = JS_VALUE_GET_TAG(op2);
    if ((tag1 == JS_TAG_INT || JS_TAG_IS_FLOAT64(tag1)) && (tag2 == JS_TAG_INT || JS_TAG_IS_FLOAT64(tag2))) {
        return tsn_add_slow_numbers(ctx, op1, op2);
    } else {
        op1 = JS_ToPrimitiveFree_tsn(ctx, JS_DupValue(ctx, op1), HINT_NONE);
        if (JS_IsException(op1) != 0) {
            JS_FreeValue(ctx, op2);
            return JS_EXCEPTION;
        }
        op2 = JS_ToPrimitiveFree_tsn(ctx, JS_DupValue(ctx, op2), HINT_NONE);
        if (JS_IsException(op2) != 0) {
            JS_FreeValue(ctx, op1);
            return JS_EXCEPTION;
        }
        tag1 = JS_VALUE_GET_TAG(op1);
        tag2 = JS_VALUE_GET_TAG(op2);
        if (tag1 == static_cast<uint32_t>(JS_TAG_STRING) || tag2 == static_cast<uint32_t>(JS_TAG_STRING)) {
            return JS_ConcatString_tsn(ctx, op1, op2);
        } else {
            return tsn_add_slow_numbers(ctx, op1, op2);
        }
    }
}

enum class tsn_unary_arith_slow_op {
    inc,
    dec,
    plus,
    neg,
};

tsn_value inline tsn_unary_arith_slow(tsn_vm* ctx, tsn_unary_arith_slow_op op, tsn_value op1) {
    double d;

    if (unlikely(JS_ToFloat64Free_tsn(ctx, &d, op1))) {
        return JS_UNDEFINED;
    }
    switch (op) {
        case tsn_unary_arith_slow_op::inc:
            d++;
            break;
        case tsn_unary_arith_slow_op::dec:
            d--;
            break;
        case tsn_unary_arith_slow_op::plus:
            break;
        case tsn_unary_arith_slow_op::neg:
            d = -d;
            break;
        default:
            abort();
    }
    return JS_NewFloat64(ctx, d);
}

enum class tsn_relational_slow_op {
    lt,
    lte,
    gt,
    gte,
};

js_force_inline tsn_value tsn_relational_slow(JSContext* ctx, tsn_relational_slow_op op, tsn_value op1, tsn_value op2) {
    bool res;

    op1 = JS_ToPrimitiveFree_tsn(ctx, op1, HINT_NUMBER);
    if (JS_IsException(op1) != 0) {
        JS_FreeValue(ctx, op2);
        return JS_EXCEPTION;
    }
    op2 = JS_ToPrimitiveFree_tsn(ctx, op2, HINT_NUMBER);
    if (JS_IsException(op2) != 0) {
        JS_FreeValue(ctx, op1);
        return JS_EXCEPTION;
    }
    if (JS_VALUE_GET_TAG(op1) == JS_TAG_STRING && JS_VALUE_GET_TAG(op2) == JS_TAG_STRING) {
        int compareResult = JS_StringCompareUnsafe_tsn(ctx, op1, op2);
        JS_FreeValue(ctx, op1);
        JS_FreeValue(ctx, op2);
        switch (op) {
            case tsn_relational_slow_op::lt:
                res = (compareResult < 0);
                break;
            case tsn_relational_slow_op::lte:
                res = (compareResult <= 0);
                break;
            case tsn_relational_slow_op::gt:
                res = (compareResult > 0);
                break;
            default:
            case tsn_relational_slow_op::gte:
                res = (compareResult >= 0);
                break;
        }
    } else {
        double d1;
        double d2;
        if (JS_ToFloat64Free_tsn(ctx, &d1, op1) != 0) {
            JS_FreeValue(ctx, op2);
            return JS_EXCEPTION;
        }
        if (JS_ToFloat64Free_tsn(ctx, &d2, op2) != 0) {
            return JS_EXCEPTION;
        }
        switch (op) {
            case tsn_relational_slow_op::lt:
                res = (d1 < d2); /* if NaN return false */
                break;
            case tsn_relational_slow_op::lte:
                res = (d1 <= d2); /* if NaN return false */
                break;
            case tsn_relational_slow_op::gt:
                res = (d1 > d2); /* if NaN return false */
                break;
            default:
            case tsn_relational_slow_op::gte:
                res = (d1 >= d2); /* if NaN return false */
                break;
        }
    }
    return JS_NewBool(ctx, static_cast<JS_BOOL>(res));
}

// TODO(rjaber): Copy static no_inline int js_relational_slow(JSContext *ctx, JSValue *sp, OPCodeEnum op)

// TODO(rjaber): Copy static no_inline int js_relational_slow(JSContext *ctx, JSValue *sp, OPCodeEnum op)
tsn_value tsn_op_lt(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_RelationalSlow_tsn(ctx, op1, op2, JS_OP_LT);
}

tsn_value tsn_op_lte(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_RelationalSlow_tsn(ctx, op1, op2, JS_OP_LTE);
}

tsn_value tsn_op_lte_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_RelationalSlow_tsn(ctx, op1, op2, JS_OP_LTE);
}

tsn_value tsn_op_gt(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_RelationalSlow_tsn(ctx, op1, op2, JS_OP_GT);
}

tsn_value tsn_op_gte(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_RelationalSlow_tsn(ctx, op1, op2, JS_OP_GTE);
}

tsn_value tsn_op_gte_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_RelationalSlow_tsn(ctx, op1, op2, JS_OP_GTE);
}

tsn_value tsn_op_eq(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_EqualSlow_tsn(ctx, op1, op2, 0);
}

tsn_value tsn_op_eq_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    auto value = JS_StrictEqualSlow_tsn(ctx, op1, op2);
    return tsn_new_bool(ctx, static_cast<bool>(value));
}

tsn_value tsn_op_ne(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    return JS_EqualSlow_tsn(ctx, op1, op2, 1);
}

tsn_value tsn_op_ne_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    auto value = JS_StrictEqualSlow_tsn(ctx, op1, op2);
    return tsn_new_bool(ctx, value == 0);
}

tsn_value tsn_op_div(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    if (likely(JS_VALUE_IS_BOTH_INT(op1, op2))) {
        int v1;
        int v2;
        if (JS_IsMathMode_tsn(ctx) != 0) {
            return tsn_binary_arith_slow(ctx, tsn_binary_arith_slow_op::div, op1, op2);
        }
        v1 = JS_VALUE_GET_INT(op1);
        v2 = JS_VALUE_GET_INT(op2);
        return JS_NewFloat64(ctx, (double)v1 / (double)v2);
    } else {
        return tsn_binary_arith_slow(ctx, tsn_binary_arith_slow_op::div, op1, op2);
    }
}

tsn_value tsn_op_mult(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    double d;
    if (likely(JS_VALUE_IS_BOTH_INT(op1, op2))) {
        int32_t v1;
        int32_t v2;
        int64_t r;
        v1 = JS_VALUE_GET_INT(op1);
        v2 = JS_VALUE_GET_INT(op2);
        r = (int64_t)v1 * v2;
        if (unlikely((int)r != r)) {
            d = (double)r;
            return __JS_NewFloat64(ctx, d);
        }
        /* need to test zero case for -0 result */
        if (unlikely(r == 0 && (v1 | v2) < 0)) {
            d = -0.0;
            return __JS_NewFloat64(ctx, d);
        }
        return JS_NewInt32(ctx, r);
    } else if (JS_VALUE_IS_BOTH_FLOAT(op1, op2)) {
        d = JS_VALUE_GET_FLOAT64(op1) * JS_VALUE_GET_FLOAT64(op2);
        return __JS_NewFloat64(ctx, d);
    } else {
        return tsn_binary_arith_slow(ctx, tsn_binary_arith_slow_op::mul, op1, op2);
    }
}

tsn_value tsn_op_add(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    if (likely(JS_VALUE_IS_BOTH_INT(op1, op2))) {
        int64_t r;
        r = (int64_t)JS_VALUE_GET_INT(op1) + JS_VALUE_GET_INT(op2);
        if (unlikely((int)r != r)) {
            return tsn_add_slow(ctx, op1, op2);
        }
        return JS_NewInt32(ctx, r);
    } else if (JS_VALUE_IS_BOTH_FLOAT(op1, op2)) {
        return __JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) + JS_VALUE_GET_FLOAT64(op2));
    } else {
        return tsn_add_slow(ctx, op1, op2);
    }
}

tsn_value tsn_op_sub(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    if (likely(JS_VALUE_IS_BOTH_INT(op1, op2))) {
        int64_t r;
        r = (int64_t)JS_VALUE_GET_INT(op1) - JS_VALUE_GET_INT(op2);
        if (unlikely((int)r != r)) {
            return tsn_binary_arith_slow(ctx, tsn_binary_arith_slow_op::sub, op1, op2);
        }
        return JS_NewInt32(ctx, r);
    } else if (JS_VALUE_IS_BOTH_FLOAT(op1, op2)) {
        return __JS_NewFloat64(ctx, JS_VALUE_GET_FLOAT64(op1) - JS_VALUE_GET_FLOAT64(op2));
    } else {
        return tsn_binary_arith_slow(ctx, tsn_binary_arith_slow_op::sub, op1, op2);
    }
}

tsn_value tsn_op_inc(tsn_vm* ctx, tsn_value op1) {
    int val;
    if (JS_VALUE_GET_TAG(op1) == JS_TAG_INT) {
        val = JS_VALUE_GET_INT(op1);
        if (unlikely(val == INT32_MAX)) {
            return tsn_unary_arith_slow(ctx, tsn_unary_arith_slow_op::inc, op1);
        }
        return JS_NewInt32(ctx, val + 1);
    } else {
        return tsn_unary_arith_slow(ctx, tsn_unary_arith_slow_op::inc, op1);
    }
}

tsn_value tsn_op_dec(tsn_vm* ctx, tsn_value op1) {
    if (JS_VALUE_GET_TAG(op1) == JS_TAG_INT) {
        int64_t r;
        r = (int64_t)JS_VALUE_GET_INT(op1) - 1;
        if (unlikely((int)r != r)) {
            return tsn_unary_arith_slow(ctx, tsn_unary_arith_slow_op::dec, op1);
        }
        return JS_NewInt32(ctx, r);
    } else {
        return tsn_unary_arith_slow(ctx, tsn_unary_arith_slow_op::dec, op1);
    }
}

tsn_value tsn_op_bnot(tsn_vm* ctx, tsn_value op1) {
    if (JS_VALUE_GET_TAG(op1) == JS_TAG_INT) {
        return JS_NewInt32(ctx, ~JS_VALUE_GET_INT(op1));
    } else {
        uint32_t v1;
        if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
            return JS_EXCEPTION;
        }
        return JS_NewInt32(ctx, ~v1);
    }
}

tsn_value tsn_op_lnot(tsn_vm* ctx, tsn_value op1) {
    return tsn_new_bool(ctx, !tsn_to_bool(ctx, op1));
}

tsn_value tsn_op_typeof(tsn_vm* ctx, tsn_value op1) {
    return JS_TypeOf_tsn(ctx, op1);
}

tsn_value tsn_op_neg(tsn_vm* ctx, tsn_value op1) {
    uint32_t tag;
    int val;
    double d;
    tag = JS_VALUE_GET_TAG(op1);
    if (tag == JS_TAG_INT) {
        val = JS_VALUE_GET_INT(op1);
        /* Note: -0 cannot be expressed as integer */
        if (unlikely(val == 0)) {
            d = -0.0;
            return __JS_NewFloat64(ctx, d);
        }
        if (unlikely(val == INT32_MIN)) {
            d = -(double)val;
            return __JS_NewFloat64(ctx, d);
        }
        return JS_NewInt32(ctx, -val);
    } else if (JS_TAG_IS_FLOAT64(tag)) {
        d = -JS_VALUE_GET_FLOAT64(op1);
        return __JS_NewFloat64(ctx, d);
    } else {
        return tsn_unary_arith_slow(ctx, tsn_unary_arith_slow_op::neg, op1);
    }
}

tsn_value tsn_op_plus(tsn_vm* ctx, tsn_value op1) {
    uint32_t tag;
    tag = JS_VALUE_GET_TAG(op1);

    if (tag == JS_TAG_INT || tag == JS_TAG_FLOAT64) {
        return op1;
    }

    double d1;

    if (unlikely(JS_ToFloat64Free_tsn(ctx, &d1, JS_DupValue(ctx, op1)))) {
        return JS_EXCEPTION;
    }

    return tsn_double(ctx, d1);
}

tsn_value tsn_op_ls(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t v1;
    uint32_t v2;
    uint32_t r;
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v2, op2))) {
        return JS_EXCEPTION;
    }

    r = v1 << (v2 & 0x1f);

    return JS_NewInt32(ctx, r);
}

tsn_value tsn_op_rs(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t v1;
    uint32_t v2;
    uint32_t r;
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v2, op2))) {
        return JS_EXCEPTION;
    }

    r = (int)v1 >> (v2 & 0x1f);

    return JS_NewInt32(ctx, r);
}

tsn_value tsn_op_urs(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t v1;
    uint32_t v2;
    uint32_t r;
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v2, op2))) {
        return JS_EXCEPTION;
    }

    r = v1 >> (v2 & 0x1f);

    return JS_NewUint32(ctx, r);
}

tsn_value tsn_op_bxor(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t v1;
    uint32_t v2;
    uint32_t r;
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v2, op2))) {
        return JS_EXCEPTION;
    }

    r = v1 ^ v2;

    return JS_NewInt32(ctx, r);
}

tsn_value tsn_op_bor(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t v1;
    uint32_t v2;
    uint32_t r;
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v2, op2))) {
        return JS_EXCEPTION;
    }

    r = v1 | v2;

    return JS_NewInt32(ctx, r);
}

tsn_value tsn_op_band(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t v1;
    uint32_t v2;
    uint32_t r;
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v1, op1))) {
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToInt32(ctx, (int32_t*)&v2, op2))) {
        return JS_EXCEPTION;
    }

    r = v1 & v2;

    return JS_NewInt32(ctx, r);
}

tsn_value tsn_op_exp(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    // TODO(simon): Should support big int as well.
    double d1;
    double d2;
    double r;

    if (unlikely(JS_ToFloat64Free_tsn(ctx, &d1, op1))) {
        JS_FreeValue(ctx, op2);
        return JS_EXCEPTION;
    }
    if (unlikely(JS_ToFloat64Free_tsn(ctx, &d2, op2))) {
        return JS_EXCEPTION;
    }

    r = std::pow(d1, d2);
    return JS_NewFloat64(ctx, r);
}

tsn_value tsn_op_mod(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    uint32_t tag1;
    uint32_t tag2;
    double d1;
    double d2;

    tag1 = JS_VALUE_GET_NORM_TAG(op1);
    tag2 = JS_VALUE_GET_NORM_TAG(op2);
    /* fast path for float operations */
    if (tag1 == JS_TAG_FLOAT64 && tag2 == JS_TAG_FLOAT64) {
        d1 = JS_VALUE_GET_FLOAT64(op1);
        d2 = JS_VALUE_GET_FLOAT64(op2);
    } else if (tag1 == JS_TAG_INT && tag2 == JS_TAG_INT) {
        auto v1 = JS_VALUE_GET_INT(op1);
        auto v2 = JS_VALUE_GET_INT(op2);
        if (v1 < 0 || v2 <= 0) {
            return tsn_double(ctx, std::fmod(v1, v2));
        } else {
            return tsn_int64(ctx, (int64_t)v1 % (int64_t)v2);
        }
    } else {
        if (unlikely(JS_ToFloat64Free_tsn(ctx, &d1, op1) != 0)) {
            JS_FreeValue(ctx, op2);
            return JS_EXCEPTION;
        }
        if (unlikely(JS_ToFloat64Free_tsn(ctx, &d2, op2) != 0)) {
            return JS_EXCEPTION;
        }
    }

    return tsn_double(ctx, std::fmod(d1, d2));
}

tsn_value tsn_op_instanceof(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    auto b = JS_IsInstanceOf(ctx, op1, op2);
    if (b == -1) {
        return JS_EXCEPTION;
    }
    return tsn_new_bool(ctx, static_cast<bool>(b));
}

tsn_value tsn_op_in(tsn_vm* ctx, tsn_value op1, tsn_value op2) {
    if (JS_VALUE_GET_TAG(op2) != JS_TAG_OBJECT) {
        return JS_ThrowTypeError(ctx, "invalid 'in' operand");
    }
    auto atom = JS_ValueToAtom(ctx, op1);
    if (unlikely(atom == JS_ATOM_NULL)) {
        return tsn_exception(ctx);
    }
    auto ret = JS_HasProperty(ctx, op2, atom);
    JS_FreeAtom(ctx, atom);
    if (ret < 0) {
        return tsn_exception(ctx);
    }
    return JS_NewBool(ctx, ret);
}

static bool is_ignored_atom(tsn_atom atom, int atoms_to_ignore_size, const tsn_atom* atoms_to_ignore) {
    for (int i = 0; i < atoms_to_ignore_size; i++) {
        if (atom == atoms_to_ignore[i]) {
            return true;
        }
    }
    return false;
}

static tsn_value tsn_copy_properties_impl(tsn_vm* ctx,
                                          tsn_value_const dest_obj,
                                          tsn_value_const src_obj,
                                          int atoms_to_ignore_size,
                                          tsn_atom* atoms_to_ignore) {
    if (JS_IsArray(ctx, dest_obj) != 0) {
        auto index_start = JS_GetArrayLength_tsn(ctx, dest_obj);
        // Source object is array
        if (JS_IsArray(ctx, src_obj) != 0) {
            auto array_length = JS_GetArrayLength_tsn(ctx, src_obj);

            for (int i = 0; i < array_length; i++) {
                auto item = tsn_get_property_index(ctx, src_obj, static_cast<uint32_t>(i));
                if (tsn_is_exception(ctx, item)) {
                    return JS_EXCEPTION;
                }

                if (tsn_set_property_index(ctx, dest_obj, static_cast<uint32_t>(index_start + i), item) < 0) {
                    tsn_release(ctx, item);
                    return JS_EXCEPTION;
                }
                tsn_release(ctx, item);
            }

            return tsn_int32(ctx, array_length);
        }
        // Source object is iterable
        tsn_iterator iterator = tsn_new_iterator(ctx, src_obj);
        if (!tsn_iterator_is_exception(ctx, &iterator)) {
            int32_t i = 0;
            for (auto item = tsn_iterator_next(ctx, &iterator); tsn_iterator_has_value(ctx, &iterator);
                 item = tsn_iterator_next(ctx, &iterator)) {
                if (tsn_is_exception(ctx, item)) {
                    tsn_release_iterator(ctx, &iterator);
                    return JS_EXCEPTION;
                }
                if (tsn_set_property_index(ctx, dest_obj, static_cast<uint32_t>(index_start + (i++)), item) < 0) {
                    tsn_release(ctx, item);
                    tsn_release_iterator(ctx, &iterator);
                    return JS_EXCEPTION;
                }
                tsn_release(ctx, item);
            }
            tsn_release_iterator(ctx, &iterator);
            return tsn_int32(ctx, i);
        } else {
            // Not iterable
            return JS_EXCEPTION;
        }
    } else {
        if (tsn_is_undefined_or_null(ctx, src_obj)) {
            // spread undefined or null copies nothing
            return tsn_int32(ctx, 0);
        }
        JSPropertyEnum* properties = nullptr;
        uint32_t properties_length;
        if (JS_GetOwnPropertyNames(
                ctx, &properties, &properties_length, src_obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) != 0) {
            return JS_EXCEPTION;
        }

        int32_t added_properties = 0;

        for (uint32_t i = 0; i < properties_length; i++) {
            const auto& property = properties[i];
            if (atoms_to_ignore != nullptr && is_ignored_atom(property.atom, atoms_to_ignore_size, atoms_to_ignore)) {
                continue;
            }

            auto property_value = tsn_get_property0(ctx, &src_obj, property.atom);
            if (tsn_is_exception(ctx, property_value)) {
                JS_FreePropertyEnum(ctx, properties, properties_length);
                return JS_EXCEPTION;
            }

            if (tsn_set_property(ctx, &dest_obj, property.atom, &property_value) < 0) {
                tsn_release(ctx, property_value);
                JS_FreePropertyEnum(ctx, properties, properties_length);
                return JS_EXCEPTION;
            }

            tsn_release(ctx, property_value);
            added_properties++;
        }

        JS_FreePropertyEnum(ctx, properties, properties_length);

        return tsn_int32(ctx, added_properties);
    }
}

tsn_value tsn_copy_filtered_properties(
    tsn_vm* ctx, tsn_value_const dest_obj, tsn_value_const src_obj, int argc, tsn_atom* atoms_to_ignore) {
    return tsn_copy_properties_impl(ctx, dest_obj, src_obj, argc, atoms_to_ignore);
}

tsn_value tsn_copy_properties(tsn_vm* ctx, tsn_value_const dest_obj, tsn_value_const src_obj) {
    return tsn_copy_properties_impl(ctx, dest_obj, src_obj, 0, nullptr);
}

void tsn_dump_error(tsn_vm* ctx) {
    tsn_value exception_val = JS_GetException(ctx);
    tsn_dump_error1(ctx, exception_val);
    JS_FreeValue(ctx, exception_val);
}

static void tsn_update_stack_limit(tsn_runtime* runtime) {
    auto stackAttrs = Valdi::Thread::getCurrentStackAttrs();
    auto* startAddress = static_cast<void*>(static_cast<uint8_t*>(stackAttrs.minFrameAddress) + stackAttrs.stackSize);

    auto maxStackSize = static_cast<size_t>(static_cast<double>(stackAttrs.stackSize) * 0.75);

    JS_SetCStackTopPointer(runtime, startAddress);
    JS_SetMaxStackSize(runtime, maxStackSize);
}

tsn_vm* tsn_init(int /*argc*/, const char** /*argv*/) {
    auto* runtime = JS_NewRuntime();
    if (runtime == nullptr) {
        return nullptr;
    }

    tsn_update_stack_limit(runtime);

    auto* ctx = JS_NewContext(runtime);
    if (runtime == nullptr) {
        JS_FreeRuntime(runtime);
        return nullptr;
    }
    tsn_add_helpers(ctx);
    tsn_load_in_context(ctx);
    return ctx;
}

bool tsn_contains_module(const char* module_name) {
    tsn_module_info output;
    return tsn_get_module_info(module_name, &output);
}

bool tsn_get_module_info(const char* module_name, tsn_module_info* output) {
    auto moduleInfo = tsn::ModuleRegistry::getInstance().getModuleInfo(std::string_view(module_name));
    if (!moduleInfo) {
        return false;
    }
    output->name = moduleInfo.value().name;
    output->sha256 = moduleInfo.value().sha256;
    output->module_init_fn = moduleInfo.value().moduleInitFn;

    return true;
}

tsn_value tsn_load_module(tsn_vm* ctx, const char* module_name) {
    tsn_module_info module_info;
    if (!tsn_get_module_info(module_name, &module_info)) {
        JS_ThrowInternalError(ctx, "Failed to find %s in registry", module_name);
        return JS_EXCEPTION;
    }

    auto* tsn_vm_helpers = tsn_get_vm_helpers(ctx);
    const char* previousModuleName = tsn_vm_helpers->current_module_name;
    tsn_vm_helpers->current_module_name = module_info.name;
    auto result = module_info.module_init_fn(ctx);
    tsn_vm_helpers->current_module_name = previousModuleName;

    return result;
}

void tsn_fini(tsn_vm** vm) {
    if (!vm) {
        return;
    }

    auto* tsn_vm_helpers = tsn_get_vm_helpers(*vm);
    JS_FreeValue(*vm, tsn_vm_helpers->function_prototype);
    auto* runtime = JS_GetRuntime(*vm);
    js_free(*vm, tsn_vm_helpers);
    JS_FreeContext(*vm);
    JS_RunGC(runtime);
    JS_FreeRuntime(runtime);
    *vm = nullptr;
}

js_force_inline tsn_closure* tsn_get_closure_from_func(JSValueConst funcObject) {
    return reinterpret_cast<tsn_closure*>(JS_GetOpaque(funcObject, getTSNFunctionClassID()));
}

static tsn_var_ref* get_closure_stack_vrefs(tsn_closure* closure) {
    return (tsn_var_ref*)get_closure_stack(closure);
}
static tsn_value* get_closure_stack_values(tsn_closure* closure) {
    return (tsn_value*)((uint8_t*)get_closure_stack_vrefs(closure) + sizeof(tsn_var_ref) * closure->stack_vrefs);
}
static tsn_iterator* get_closure_stack_iterators(tsn_closure* closure) {
    return (tsn_iterator*)((uint8_t*)get_closure_stack_values(closure) + sizeof(tsn_value) * closure->stack_values);
}

// Throw stack overflow if we have less than 1KB remaining on the stack for the call
static constexpr size_t kFrameMarginSizeBytes = 1024;

js_force_inline JSValue call_closure_callable(tsn_vm* ctx,
                                              tsn_value_const func,
                                              tsn_value_const thisObject,
                                              int argc,
                                              tsn_value_const* argv,
                                              tsn_closure* closure) {
    JSStackFrame stackFrame;
    stackFrame.cur_func = (JSValue)func;
    stackFrame.arg_buf = nullptr;
    stackFrame.var_buf = nullptr;
    stackFrame.cur_pc = nullptr;
    stackFrame.arg_count = 0;
    stackFrame.js_mode = 0;
    stackFrame.cur_sp = nullptr;

    if (JS_PushStackFrame_tsn(ctx, &stackFrame, kFrameMarginSizeBytes) != 0) {
        return JS_EXCEPTION;
    }

    auto value = closure->callable(ctx, thisObject, &stackFrame, argc, argv, closure);
    JS_PopStackFrame_tsn(ctx, &stackFrame);
    return value;
}

static JSValue __tsn_call_trampoline(
    JSContext* context, JSValueConst funcObject, JSValueConst thisValue, int argc, JSValueConst* argv, int flags) {
    auto* closure = tsn_get_closure_from_func(funcObject);
    return call_closure_callable(context, funcObject, thisValue, argc, argv, closure);
}

static void __tsn_trampoline_finalize(JSRuntime* rt, JSValue value) {
    auto* closure = tsn_get_closure_from_func(value);
    tsn_free_closure(rt, closure);
}

static void __tsn_trampoline_mark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    auto* closure = tsn_get_closure_from_func(val);

    for (size_t i = 0; i < closure->var_refs_length; i++) {
        auto* var_ref = closure->var_refs[i];
        JS_MarkVarRef_tsn(rt, var_ref, mark_func);
    }
    // only mark stack when the function is not done.
    // stack is cleaned up when it reaches the normal return point.
    if (closure->stack_vrefs + closure->stack_values + closure->stack_iterators > 0 && closure->resume_point > 0) {
        tsn_var_ref* vrefs = get_closure_stack_vrefs(closure);
        for (size_t i = 0; i < closure->stack_vrefs; i++) {
            JS_MarkVarRef_tsn(rt, vrefs[i], mark_func);
        }
        tsn_value* values = get_closure_stack_values(closure);
        for (size_t i = 0; i < closure->stack_values; i++) {
            JS_MarkValue(rt, values[i], mark_func);
        }
        tsn_iterator* its = get_closure_stack_iterators(closure);
        for (size_t i = 0; i < closure->stack_iterators; i++) {
            JS_MarkValue(rt, its[i].iterator_value, mark_func);
            JS_MarkValue(rt, its[i].iterator_method, mark_func);
        }
    }
}

static tsn_module* tsn_get_module_from_value(JSValue value) {
    return reinterpret_cast<tsn_module*>(JS_GetOpaque(value, getTSNModuleClassID()));
}

static void __tsn_module_finalize(JSRuntime* rt, JSValue value) {
    auto* module = tsn_get_module_from_value(value);

    auto atoms_length = module->atoms_length;
    for (size_t i = 0; i < atoms_length; i++) {
        if (module->atoms[i] != 0) {
            JS_FreeAtomRT(rt, module->atoms[i]);
        }
    }
    auto constants_length = module->constants_length;
    for (size_t i = 0; i < constants_length; i++) {
        JS_FreeValueRT(rt, module->constants[i]);
    }

    js_free_rt(rt, module);
}

static void __tsn_module_mark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
    auto* module = tsn_get_module_from_value(val);
    for (size_t i = 0; i < module->constants_length; i++) {
        JS_MarkValue(rt, module->constants[i], mark_func);
    }
}

static int tsn_register_class(tsn_vm* ctx, JSClassDef* classDef, JSClassID classID) {
    return JS_NewClass(JS_GetRuntime(ctx), classID, classDef);
}

int buildBacktrace(JSContext* /*ctx*/, const JSStackFrame* sf, char* output_buf, size_t output_buf_length) {
    auto* closure = tsn_get_closure_from_func(sf->cur_func);
    if (closure == nullptr) {
        return snprintf(output_buf, output_buf_length, "native");
    }

    return snprintf(output_buf, output_buf_length, "%s:%d", closure->module->name, sf->arg_count);
}

void tsn_load_in_context(JSContext* context) {
    auto* helpers = reinterpret_cast<tsn_vm_helpers*>(js_malloc(context, sizeof(tsn_vm_helpers)));
    helpers->function_prototype = JS_UNDEFINED;
    helpers->home_object_atom = JS_ATOM_NULL;
    helpers->current_module_name = nullptr;
    JS_SetContextOpaque(context, helpers);

    auto global_object = JS_GetGlobalObject(context);

    helpers->function_prototype = JS_GetPropertyStr(context, global_object, "Function");
    helpers->home_object_atom = JS_NewAtom(context, "[[HomeObject]]");

    JS_FreeValue(context, global_object);

    {
        JSClassDef class_def;
        std::memset(&class_def, 0, sizeof(JSClassDef));
        class_def.class_name = "TSN_Function";
        class_def.call = &__tsn_call_trampoline;
        class_def.finalizer = &__tsn_trampoline_finalize;
        class_def.gc_mark = &__tsn_trampoline_mark;

        tsn_register_class(context, &class_def, getTSNFunctionClassID());
    }
    {
        JSClassDef class_def;
        std::memset(&class_def, 0, sizeof(JSClassDef));
        class_def.class_name = "TSN_Module";
        class_def.finalizer = &__tsn_module_finalize;
        class_def.gc_mark = &__tsn_module_mark;

        tsn_register_class(context, &class_def, getTSNModuleClassID());
    }

    auto* runtime = JS_GetRuntime(context);
    JS_SetBacktraceFunction(runtime, &buildBacktrace);

    // optionally load typescript native runtime library
    // (for async/await support)
    tsn_module_info mi;
    if (tsn_get_module_info("rtl/tsn", &mi)) {
        auto load_result = tsn_require_from_string(context, mi.name);
        tsn_release(context, load_result);
    }
}

void tsn_unload_in_context(JSContext* context) {
    auto* helpers = tsn_get_vm_helpers(context);
    JS_FreeValue(context, helpers->function_prototype);
    tsn_free_atom(context, helpers->home_object_atom);
    JS_SetContextOpaque(context, nullptr);

    auto* runtime = JS_GetRuntime(context);
    JS_SetBacktraceFunction(runtime, nullptr);

    js_free(context, helpers);
}

void tsn_register_module(char const* module_name, tsn_module_init_fn module_init_fn) {
    tsn_register_module_pre_launch(module_name, "", module_init_fn);
}

void tsn_register_module_pre_launch(char const* module_name, const char* sha256, tsn_module_init_fn module_init_fn) {
    tsn::ModuleRegistry::getInstance().unsafeRegisterModule(module_name, sha256, module_init_fn);
}

void tsn_register_module_post_launch(char const* module_name, const char* sha256, tsn_module_init_fn module_init_fn) {
    tsn::ModuleRegistry::getInstance().registerModule(module_name, sha256, module_init_fn);
}

tsn_closure* tsn_new_closure(tsn_vm* ctx,
                             tsn_module* module,
                             size_t vars_length,
                             uint32_t stack_vrefs,
                             uint32_t stack_values,
                             uint32_t stack_iterators) {
    tsn_retain_module(ctx, module);
    uint8_t* closure_bytes = reinterpret_cast<uint8_t*>(
        js_malloc(ctx,
                  sizeof(tsn_closure) + (sizeof(tsn_var_ref) * vars_length) + (sizeof(tsn_var_ref) * stack_vrefs) +
                      (sizeof(tsn_value) * stack_values) + (sizeof(tsn_iterator) * stack_iterators)));
    auto* closure = reinterpret_cast<tsn_closure*>(closure_bytes);
    closure->module = module;
    closure->ref_count = 1;
    closure->callable = nullptr;
    closure->is_class_constructor = false;
    closure->var_refs_length = vars_length;
    closure->resume_point = 0;
    closure->stack_vrefs = stack_vrefs;
    closure->stack_values = stack_values;
    closure->stack_iterators = stack_iterators;

    for (size_t i = 0; i < vars_length; i++) {
        closure->var_refs[i] = tsn_empty_var_ref(ctx);
    }

    return closure;
}

void __tsn_deallocate_closure(tsn_runtime* rt, tsn_closure* closure) {
    // only free stack when the function is not done.
    // stack is cleaned up when it reaches the normal return point.
    if (closure->stack_vrefs + closure->stack_values + closure->stack_iterators > 0 && closure->resume_point > 0) {
        tsn_var_ref* vrefs = get_closure_stack_vrefs(closure);
        for (size_t i = 0; i < closure->stack_vrefs; i++) {
            tsn_release_var_ref_rt(rt, vrefs[i]);
        }
        tsn_value* values = get_closure_stack_values(closure);
        for (size_t i = 0; i < closure->stack_values; i++) {
            JS_FreeValueRT(rt, values[i]);
        }
        tsn_iterator* its = get_closure_stack_iterators(closure);
        for (size_t i = 0; i < closure->stack_iterators; i++) {
            JS_FreeValueRT(rt, its[i].iterator_value);
            JS_FreeValueRT(rt, its[i].iterator_method);
        }
    }

    tsn_free_module_rt(rt, closure->module);

    for (size_t i = 0; i < closure->var_refs_length; i++) {
        tsn_release_var_ref_rt(rt, closure->var_refs[i]);
    }

    js_free_rt(rt, closure);
}

void tsn_release_vars(tsn_vm* ctx, int n, tsn_value** vars) {
    for (int i = 0; i < n; ++i) {
        JS_FreeValue(ctx, *vars[i]);
    }
}
void tsn_release_var_refs(tsn_vm* ctx, int n, tsn_var_ref* var_refs) {
    for (int i = 0; i < n; ++i) {
        JS_FreeVarRef_tsn(ctx, var_refs[i]);
    }
}
void tsn_release_iterators(tsn_vm* ctx, int n, tsn_iterator** iterators) {
    for (int i = 0; i < n; ++i) {
        tsn_release_iterator(ctx, iterators[i]);
    }
}

void tsn_throw(tsn_vm* ctx, tsn_value exception) {
    JS_Throw(ctx, JS_DupValue(ctx, exception));
}

tsn_value tsn_get_exception(tsn_vm* ctx) {
    return JS_GetException(ctx);
}

int tsn_get_ref_count(tsn_vm* /*ctx*/, tsn_value v) {
    if (JS_VALUE_HAS_REF_COUNT(v)) {
        auto* p = (JSRefCountHeader*)JS_VALUE_GET_PTR(v);
        return p->ref_count;
    } else {
        return 0;
    }
}

tsn_value tsn_delete_property(tsn_vm* ctx, tsn_value_const obj, tsn_atom prop) {
    auto result = JS_DeleteProperty(ctx, obj, prop, 0);
    if (result < 0) {
        return tsn_exception(ctx);
    } else {
        return tsn_new_bool(ctx, result != 0);
    }
}

tsn_value tsn_delete_property_value(tsn_vm* ctx, tsn_value_const obj, tsn_value_const prop) {
    return withNameAsAtomValueRet(ctx, prop, [&](tsn_atom prop) { return tsn_delete_property(ctx, obj, prop); });
}

tsn_value tsn_call(tsn_vm* ctx, tsn_value_const func_obj, tsn_value_const this_obj, int argc, tsn_value_const* argv) {
    tsn_value retval;

    auto* closure = tsn_get_closure_from_func(func_obj);
    if (closure != nullptr) {
        // Fast path for calls to TSN functions
        retval = call_closure_callable(ctx, func_obj, this_obj, argc, argv, closure);
    } else {
        // We must retain-release the arguments ourselves as QuickJS
        // might mutate the argv array in place, potentially setting up
        // different variables. It expects us to release the array after
        // a JS call is made.
        for (int i = 0; i < argc; i++) {
            tsn_retain(ctx, argv[i]);
        }

        retval = JS_CallInternal_tsn(ctx, func_obj, this_obj, JS_UNDEFINED, argc, argv, 0);

        for (int i = 0; i < argc; i++) {
            tsn_release(ctx, argv[i]);
        }
    }

    return retval;
}

tsn_value tsn_call_vargs(tsn_vm* ctx, tsn_value_const func_obj, tsn_value_const this_obj, tsn_value_const args) {
    uint32_t len;
    auto* argumentsList = JS_BuildArgumentsList_tsn(ctx, args, &len);
    if (argumentsList == nullptr) {
        return tsn_exception(ctx);
    }

    auto result = tsn_call(ctx, func_obj, this_obj, static_cast<size_t>(len), argumentsList);

    JS_FreeArgumentsList_tsn(ctx, argumentsList, len);
    return result;
}

tsn_value tsn_call_fargs(
    tsn_vm* ctx, tsn_value_const func_obj, tsn_value_const this_obj, int argc, tsn_value_const* argv) {
    return tsn_call(ctx, func_obj, this_obj, argc, argv);
}

tsn_value tsn_call_constructor(
    tsn_vm* ctx, tsn_value_const ctor, tsn_value_const new_target, int argc, tsn_value_const* argv) {
    if (tsn_is_undefined(ctx, ctor)) {
        // no ctor provided, create from prototype
        return JS_CreateFromCtor_tsn(ctx, new_target, 1);
    }
    auto* closure = tsn_get_closure_from_func(ctor);
    if (closure != nullptr) {
        // Fast path for calls to TSN functions
        if (closure->is_class_constructor) {
            // class constructor returns the newly constructed object
            return call_closure_callable(ctx, ctor, new_target, argc, argv, closure);
        } else {
            // function as class
            // create an emtpry object from the prototype
            auto obj = JS_CreateFromCtor_tsn(ctx, new_target, 1);
            // then call the function and use the empty object as 'this'
            call_closure_callable(ctx, ctor, obj, argc, argv, closure);
            return obj;
        }
    } else {
        // Call interpreted code
        return JS_CallConstructorInternal_tsn(ctx, ctor, new_target, argc, argv, 0);
    }
}

tsn_value tsn_call_constructor_vargs(tsn_vm* ctx,
                                     tsn_value_const ctor,
                                     tsn_value_const new_target,
                                     tsn_value_const args) {
    uint32_t len;
    auto* argumentsList = JS_BuildArgumentsList_tsn(ctx, args, &len);
    if (argumentsList == nullptr) {
        return tsn_exception(ctx);
    }
    auto result = tsn_call_constructor(ctx, ctor, new_target, static_cast<size_t>(len), argumentsList);
    JS_FreeArgumentsList_tsn(ctx, argumentsList, len);
    return result;
}

tsn_module* tsn_new_module(tsn_vm* ctx, size_t atoms_length, size_t constants_length, size_t prop_cache_slots) {
    auto jsModule = JS_NewObjectClass(ctx, getTSNModuleClassID());
    if (tsn_is_exception(ctx, jsModule)) {
        return nullptr;
    }

    auto* module = reinterpret_cast<tsn_module*>(js_malloc(ctx,
                                                           sizeof(tsn_module) + (sizeof(tsn_atom) * atoms_length) +
                                                               sizeof(tsn_value) * constants_length +
                                                               sizeof(tsn_prop_cache) * prop_cache_slots));
    module->name = tsn_get_vm_helpers(ctx)->current_module_name;
    module->atoms_length = atoms_length;
    module->constants_length = constants_length;
    module->prop_cache_slots = prop_cache_slots;
    module->module_as_value = jsModule;
    // Atoms point right after the tsn_module struct
    module->atoms = reinterpret_cast<tsn_atom*>(reinterpret_cast<uint8_t*>(module) + sizeof(tsn_module));
    // Constants point after the atoms
    module->constants = reinterpret_cast<tsn_value*>(reinterpret_cast<uint8_t*>(module) + sizeof(tsn_module) +
                                                     (sizeof(tsn_atom) * atoms_length));
    module->prop_cache =
        reinterpret_cast<tsn_prop_cache*>(reinterpret_cast<uint8_t*>(module) + sizeof(tsn_module) +
                                          (sizeof(tsn_atom) * atoms_length) + (sizeof(tsn_value) * constants_length));
    for (size_t i = 0; i < atoms_length; i++) {
        module->atoms[i] = 0;
    }
    for (size_t i = 0; i < constants_length; i++) {
        module->constants[i] = JS_UNINITIALIZED;
    }
    for (size_t i = 0; i < prop_cache_slots; i++) {
        memset(&module->prop_cache[i], 0, sizeof(tsn_prop_cache));
    }

    JS_SetOpaque(jsModule, module);

    return module;
}

void tsn_free_module(tsn_vm* ctx, tsn_module* module) {
    if (module != nullptr) {
        tsn_release(ctx, module->module_as_value);
    }
}

void tsn_free_module_rt(tsn_runtime* rt, tsn_module* module) {
    if (module != nullptr) {
        JS_FreeValueRT(rt, module->module_as_value);
    }
}

void tsn_retain_module(tsn_vm* ctx, tsn_module* module) {
    tsn_retain(ctx, module->module_as_value);
}

static tsn_value tsn_get_home_object(tsn_vm* ctx, tsn_value_const method) {
    static tsn_prop_cache prop_cache = {0, 0, 0, 0}; // home object has one fixed atom so we can get prop with cache
    return JS_GetProperty_tsn(ctx, method, tsn_get_vm_helpers(ctx)->home_object_atom, method, &prop_cache);
}

tsn_value tsn_get_super(tsn_vm* ctx, const tsn_stackframe* stackframe) {
    auto home_object = tsn_get_home_object(ctx, stackframe->cur_func);
    if (tsn_is_exception(ctx, home_object)) {
        return home_object;
    }

    auto parent_prototype = JS_GetPrototype(ctx, home_object);
    tsn_release(ctx, home_object);

    return parent_prototype;
}

tsn_value tsn_get_super_constructor(tsn_vm* ctx, const tsn_stackframe* stackframe) {
    return JS_GetPrototype(ctx, stackframe->cur_func);
}

tsn_var_ref tsn_new_var_ref(tsn_vm* ctx) {
    return JS_NewVarRef_tsn(ctx);
}

tsn_value tsn_load_var_ref(tsn_vm* ctx, tsn_var_ref var_ref) {
    return JS_LoadVarRef_tsn(ctx, var_ref);
}

void tsn_set_var_ref(tsn_vm* ctx, tsn_var_ref var_ref, tsn_value value) {
    JS_SetVarRef_tsn(ctx, var_ref, value);
}

tsn_var_ref tsn_retain_var_ref(tsn_vm* ctx, tsn_var_ref var_ref) {
    JS_DupVarRef_tsn(ctx, var_ref);
    return var_ref;
}

tsn_var_ref tsn_release_var_ref(tsn_vm* ctx, tsn_var_ref var_ref) {
    JS_FreeVarRef_tsn(ctx, var_ref);
    return nullptr;
}

void tsn_release_var_ref_rt(tsn_runtime* rt, tsn_var_ref var_ref) {
    JS_FreeVarRef_rt_tsn(rt, var_ref);
}

tsn_iterator tsn_empty_iterator(tsn_vm* ctx) {
    tsn_iterator it;
    it.iterator_value = tsn_undefined(ctx);
    it.iterator_method = tsn_undefined(ctx);
    it.has_dequeued_value = false;

    return it;
}

tsn_iterator tsn_new_iterator(tsn_vm* ctx, tsn_value iterable) {
    tsn_iterator it;
    it.iterator_method = tsn_undefined(ctx);
    it.iterator_value = JS_GetIterator_tsn(ctx, iterable, &it.iterator_method);
    it.has_dequeued_value = false;

    return it;
}

tsn_iterator tsn_new_keys_iterator(tsn_vm* ctx, tsn_value iterable) {
    tsn_iterator it;
    it.iterator_method = tsn_undefined(ctx);
    it.iterator_value = JS_GetKeysIterator_tsn(ctx, iterable);
    it.has_dequeued_value = false;

    return it;
}

tsn_iterator tsn_retain_iterator(tsn_vm* ctx, tsn_iterator* iterator) {
    tsn_retain(ctx, iterator->iterator_value);
    tsn_retain(ctx, iterator->iterator_method);
    return *iterator;
}

tsn_iterator tsn_release_iterator(tsn_vm* ctx, tsn_iterator* iterator) {
    tsn_release(ctx, iterator->iterator_value);
    tsn_release(ctx, iterator->iterator_method);
    iterator->iterator_value = JS_UNDEFINED;
    iterator->iterator_method = JS_UNDEFINED;
    iterator->has_dequeued_value = false;
    return *iterator;
}

tsn_value tsn_iterator_next(tsn_vm* ctx, tsn_iterator* iterator) {
    int done = 0;
    auto nextValue = JS_IteratorNext_tsn(ctx, iterator->iterator_value, iterator->iterator_method, &done);
    iterator->has_dequeued_value = done == 0;

    return nextValue;
}

tsn_value tsn_keys_iterator_next(tsn_vm* ctx, tsn_iterator* iterator) {
    int done = 0;
    auto nextValue = JS_KeysIteratorNext_tsn(ctx, iterator->iterator_value, &done);
    iterator->has_dequeued_value = done == 0;

    return nextValue;
}

enum class tsn_generator_op {
    gen_next,
    gen_return,
    gen_throw,
};

JSValue tsn_iterator_result(JSContext* ctx, bool done, JSValue val) {
    auto res = JS_NewObject(ctx);
    JS_SetProperty(ctx, res, JS_ATOM_done_tsn, JS_NewBool(ctx, static_cast<int>(done)));
    JS_SetProperty(ctx, res, JS_ATOM_value_tsn, JS_DupValue(ctx, val));
    return res;
}

static JSValue tsn_generator_iterator(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    return JS_DupValue(ctx, this_val);
}

static JSValue tsn_generator_next(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValue* func_data) {
    auto* closure = tsn_get_closure_from_func(func_data[0]);

    if (closure->resume_point >= 0) { // generating
        switch (static_cast<tsn_generator_op>(magic)) {
            case tsn_generator_op::gen_throw:
                // throw the error in the current js context and fallthrough
                tsn_throw(ctx, argv[0]);
                [[fallthrough]];
            case tsn_generator_op::gen_next: {
                auto res = call_closure_callable(ctx, func_data[0], this_val, argc, argv, closure);
                if (JS_IsException(res) != 0) {
                    return res;
                }
                auto iterator_result = tsn_iterator_result(ctx, closure->resume_point < 0, res);
                tsn_release_inline(ctx, res);
                return iterator_result;
            }
            case tsn_generator_op::gen_return:
                closure->resume_point = -1;
                return tsn_iterator_result(ctx, true, argv[0]);
        }
    } else { // completed
        switch (static_cast<tsn_generator_op>(magic)) {
            case tsn_generator_op::gen_next:
                return tsn_iterator_result(ctx, true, JS_UNDEFINED);
            case tsn_generator_op::gen_return:
                return tsn_iterator_result(ctx, true, argv[0]);
            case tsn_generator_op::gen_throw:
                tsn_throw(ctx, JS_DupValue(ctx, argv[0]));
        }
    }
    // should never get here
    return JS_UNDEFINED;
}

tsn_value tsn_new_generator(tsn_vm* ctx, tsn_value closure) {
    auto gen = JS_NewObject(ctx);
    JS_SetProperty(
        ctx,
        gen,
        JS_ATOM_Symbol_iterator_tsn,
        JS_NewCFunctionData(
            ctx, &tsn_generator_iterator, 0 /*# of args*/, 0 /*magic*/, 0 /*# of data vars*/, nullptr /*data*/));
    JS_SetProperty(ctx,
                   gen,
                   JS_ATOM_next_tsn,
                   JS_NewCFunctionData(ctx,
                                       &tsn_generator_next,
                                       1 /*# of args*/,
                                       static_cast<int>(tsn_generator_op::gen_next) /*magic*/,
                                       1 /*# of data vars*/,
                                       &closure /*data*/));
    JS_SetProperty(ctx,
                   gen,
                   JS_ATOM_return_tsn,
                   JS_NewCFunctionData(ctx,
                                       &tsn_generator_next,
                                       1 /*# of args*/,
                                       static_cast<int>(tsn_generator_op::gen_return) /*magic*/,
                                       1 /*# of data vars*/,
                                       &closure /*data*/));
    JS_SetProperty(ctx,
                   gen,
                   JS_ATOM_throw_tsn,
                   JS_NewCFunctionData(ctx,
                                       &tsn_generator_next,
                                       1 /*# of args*/,
                                       static_cast<int>(tsn_generator_op::gen_throw) /*magic*/,
                                       1 /*# of data vars*/,
                                       &closure /*data*/));
    // JS_SetPropertyStr(ctx, gen, "[Symbol.toStringTag]"
    return gen;
}

static tsn_value tsn_new_func_helper(tsn_vm* ctx,
                                     tsn_module* module,
                                     tsn_atom name,
                                     bool isConstructor,
                                     tsn_value_const prototype,
                                     tsn_generic_func* func,
                                     int32_t argc,
                                     size_t var_refs_length,
                                     tsn_var_ref* var_refs,
                                     uint32_t stack_vrefs = 0,
                                     uint32_t stack_values = 0,
                                     uint32_t stack_iterators = 0) {
    auto* closure = tsn_new_closure(ctx, module, var_refs_length, stack_vrefs, stack_values, stack_iterators);
    closure->callable = func;

    auto classID = getTSNFunctionClassID();
    auto js_func = JS_NewObjectProtoClass(ctx, prototype, classID);
    if (tsn_is_exception(ctx, js_func)) {
        tsn_free_closure(tsn_get_runtime(ctx), closure);
    } else {
        for (size_t i = 0; i < var_refs_length; i++) {
            closure->var_refs[i] = tsn_retain_var_ref(ctx, var_refs[i]);
        }

        JS_SetOpaque(js_func, closure);
        JS_SetFunctionLength_tsn(ctx, js_func, argc);
        if (isConstructor) {
            JS_SetFunctionName_tsn(ctx, js_func, name);
            JS_SetFunctionAsConstructor_tsn(ctx, js_func);
        }
    }

    return js_func;
}

static tsn_value tsn_new_func_helper(tsn_vm* ctx,
                                     tsn_module* module,
                                     tsn_atom name,
                                     bool isConstructor,
                                     tsn_generic_func* func,
                                     int32_t argc,
                                     size_t var_refs_length,
                                     tsn_var_ref* var_refs,
                                     uint32_t stack_vrefs = 0,
                                     uint32_t stack_values = 0,
                                     uint32_t stack_iterators = 0) {
    return tsn_new_func_helper(ctx,
                               module,
                               name,
                               isConstructor,
                               tsn_get_vm_helpers(ctx)->function_prototype,
                               func,
                               argc,
                               var_refs_length,
                               var_refs,
                               stack_vrefs,
                               stack_values,
                               stack_iterators);
}

tsn_value tsn_new_arrow_function(tsn_vm* ctx,
                                 tsn_module* module,
                                 tsn_generic_func* func,
                                 int32_t argc,
                                 uint32_t stack_vrefs,
                                 uint32_t stack_values,
                                 uint32_t stack_iterators) {
    return tsn_new_func_helper(
        ctx, module, 0, false, func, argc, 0, nullptr, stack_vrefs, stack_values, stack_iterators);
}

tsn_value tsn_new_arrow_function_closure(tsn_vm* ctx,
                                         tsn_module* module,
                                         tsn_generic_func* func,
                                         int32_t argc,
                                         size_t vars_length,
                                         tsn_var_ref* vars,
                                         uint32_t stack_vrefs,
                                         uint32_t stack_values,
                                         uint32_t stack_iterators) {
    return tsn_new_func_helper(
        ctx, module, 0, false, func, argc, vars_length, vars, stack_vrefs, stack_values, stack_iterators);
}

tsn_value tsn_new_function(tsn_vm* ctx, tsn_module* module, tsn_atom name, tsn_generic_func* func, int32_t argc) {
    return tsn_new_func_helper(ctx, module, name, true, func, argc, 0, nullptr);
}

tsn_value tsn_new_function_closure(tsn_vm* ctx,
                                   tsn_module* module,
                                   tsn_atom name,
                                   tsn_generic_func* func,
                                   int32_t argc,
                                   size_t vars_length,
                                   tsn_var_ref* vars) {
    return tsn_new_func_helper(ctx, module, name, true, func, argc, vars_length, vars);
}

tsn_value tsn_new_module_init_fn(tsn_vm* ctx, tsn_module* module, tsn_generic_func* func) {
    return tsn_new_func_helper(ctx, module, 0, false, func, 0, 0, nullptr);
}

void tsn_dump_refcount(tsn_value_const val, char const* name) {
    printf("Count(%s):", name);
    if (JS_VALUE_HAS_REF_COUNT(val)) {
        auto* p = (JSRefCountHeader*)JS_VALUE_GET_PTR(val);
        printf("%d\n", p->ref_count);
    } else {
        printf("inf\n");
    }
}

static bool tsn_inherit_prototype(tsn_vm* ctx, tsn_value_const parent_ctor, tsn_value_const child_ctor) {
    if (JS_IsUndefined(parent_ctor) != 0) {
        return true;
    }

    auto parent_prototype = JS_GetProperty(ctx, parent_ctor, JS_GetPrototypeAtom_tsn(ctx));
    if (JS_IsException(parent_prototype) != 0) {
        return false;
    }

    // Create prototype object for the child
    auto child_prototype = JS_NewObjectProto(ctx, parent_prototype);
    if (JS_IsException(child_prototype) != 0) {
        JS_FreeValue(ctx, parent_prototype);
        return false;
    }

    // Set reference from prototype object to the constructor
    if (JS_DefinePropertyValue(ctx,
                               child_prototype,
                               JS_GetConstructorAtom_tsn(ctx),
                               JS_DupValue(ctx, child_ctor),
                               JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_THROW) < 0) {
        JS_FreeValue(ctx, parent_prototype);
        JS_FreeValue(ctx, child_prototype);
        return false;
    }

    // Set the prototype property on the ctor
    if (JS_DefinePropertyValue(
            ctx, child_ctor, JS_GetPrototypeAtom_tsn(ctx), JS_DupValue(ctx, child_prototype), JS_PROP_THROW) < 0) {
        JS_FreeValue(ctx, parent_prototype);
        JS_FreeValue(ctx, child_prototype);
        return false;
    }

    JS_FreeValue(ctx, parent_prototype);
    JS_FreeValue(ctx, child_prototype);
    return true;
}

tsn_value tsn_new_class(
    tsn_vm* ctx, tsn_module* module, tsn_atom name, tsn_generic_func* func, int32_t argc, tsn_value_const parent_ctor) {
    auto js_func = tsn_new_func_helper(ctx, module, name, true, parent_ctor, func, argc, 0, nullptr);

    if (!tsn_inherit_prototype(ctx, parent_ctor, js_func)) {
        tsn_release(ctx, js_func);
        return JS_EXCEPTION;
    }

    auto* closure = tsn_get_closure_from_func(js_func);
    closure->is_class_constructor = true;

    return js_func;
}

tsn_value tsn_new_class_closure(tsn_vm* ctx,
                                tsn_module* module,
                                tsn_atom name,
                                tsn_generic_func* func,
                                int32_t argc,
                                tsn_value_const parent_ctor,
                                size_t vars_length,
                                tsn_var_ref* vars) {
    auto js_func = tsn_new_func_helper(ctx, module, name, true, parent_ctor, func, argc, vars_length, vars);
    if (!tsn_inherit_prototype(ctx, parent_ctor, js_func)) {
        tsn_release(ctx, js_func);
        return JS_EXCEPTION;
    }
    // mark function as class constructor.  class constructors return new
    // objects.  this is different from using plain function as constructors, in
    // which case we need to provide an empty object as the 'this' object before
    // calling the function.
    auto* closure = tsn_get_closure_from_func(js_func);
    closure->is_class_constructor = true;
    return js_func;
}

static tsn_result tsn_set_home_object(tsn_vm* ctx, tsn_value proto, tsn_value method) {
    return tsn_process_result(ctx,
                              JS_DefinePropertyValue(ctx,
                                                     method,
                                                     tsn_get_vm_helpers(ctx)->home_object_atom,
                                                     JS_DupValue(ctx, proto),
                                                     JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_THROW));
}

static tsn_result tsn_do_set_class_method(tsn_vm* ctx, tsn_value proto, tsn_atom name, tsn_value method) {
    if (tsn_set_home_object(ctx, proto, method) != tsn_result_success) {
        return tsn_result_failure;
    }

    return tsn_process_result(
        ctx,
        JS_DefinePropertyValue(
            ctx, proto, name, JS_DupValue(ctx, method), JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_THROW));
}

static tsn_result tsn_do_set_class_property(
    tsn_vm* ctx, tsn_value proto, tsn_atom name, const tsn_value* getter, const tsn_value* setter) {
    if (getter != nullptr) {
        if (tsn_set_home_object(ctx, proto, *getter) < 0) {
            return tsn_result_failure;
        }
    }
    if (setter != nullptr) {
        if (tsn_set_home_object(ctx, proto, *setter) < 0) {
            return tsn_result_failure;
        }
    }

    JSPropertyDescriptor desc;
    auto result = JS_GetOwnProperty(ctx, &desc, proto, name);

    if (result < 0) {
        return -1;
    }

    if (result == 0) {
        // Property does not exist yet
        desc.flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE;
        desc.value = tsn_undefined(ctx);
        desc.getter = tsn_undefined(ctx);
        desc.setter = tsn_undefined(ctx);
    }

    if (getter != nullptr) {
        desc.getter = JS_DupValue(ctx, *getter);
    }
    if (setter != nullptr) {
        desc.setter = JS_DupValue(ctx, *setter);
    }

    return tsn_process_result(
        ctx, JS_DefinePropertyGetSet(ctx, proto, name, desc.getter, desc.setter, desc.flags | JS_PROP_THROW));
}

template<typename F>
static inline tsn_result withPrototypeProperty(tsn_vm* ctx, tsn_value cls, F&& cb) {
    auto prototype = JS_GetProperty(ctx, cls, JS_GetPrototypeAtom_tsn(ctx));
    if (tsn_is_exception(ctx, prototype)) {
        return false;
    }

    auto result = cb(prototype);

    tsn_release(ctx, prototype);

    return result;
}

tsn_result tsn_set_class_method(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value method) {
    return withPrototypeProperty(
        ctx, cls, [&](tsn_value prototype) -> int { return tsn_do_set_class_method(ctx, prototype, name, method); });
}

tsn_result tsn_set_class_method_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value method) {
    return withNameAsAtom(
        ctx, name, [&](tsn_atom name) -> int { return tsn_set_class_method(ctx, cls, name, method); });
}

tsn_result tsn_set_class_static_method(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value method) {
    return tsn_do_set_class_method(ctx, cls, name, method);
}

tsn_result tsn_set_class_static_method_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value method) {
    return withNameAsAtom(
        ctx, name, [&](tsn_atom name) -> int { return tsn_set_class_static_method(ctx, cls, name, method); });
}

tsn_result tsn_set_class_property_getter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value getter) {
    return withPrototypeProperty(ctx, cls, [&](tsn_value prototype) -> int {
        return tsn_do_set_class_property(ctx, prototype, name, &getter, nullptr);
    });
}

tsn_result tsn_set_class_property_getter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value getter) {
    return withNameAsAtom(
        ctx, name, [&](tsn_atom name) -> int { return tsn_set_class_property_getter(ctx, cls, name, getter); });
}

tsn_result tsn_set_class_static_property_getter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value getter) {
    return tsn_do_set_class_property(ctx, cls, name, &getter, nullptr);
}

tsn_result tsn_set_class_static_property_getter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value getter) {
    return withNameAsAtom(
        ctx, name, [&](tsn_atom name) -> int { return tsn_set_class_static_property_getter(ctx, cls, name, getter); });
}

tsn_result tsn_set_class_property_setter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value setter) {
    return withPrototypeProperty(ctx, cls, [&](tsn_value prototype) -> int {
        return tsn_do_set_class_property(ctx, prototype, name, nullptr, &setter);
    });
}

tsn_result tsn_set_class_property_setter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value setter) {
    return withNameAsAtom(
        ctx, name, [&](tsn_atom name) -> int { return tsn_set_class_property_setter(ctx, cls, name, setter); });
}

tsn_result tsn_set_class_static_property_setter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value setter) {
    return tsn_do_set_class_property(ctx, cls, name, nullptr, &setter);
}

tsn_result tsn_set_class_static_property_setter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value setter) {
    return withNameAsAtom(
        ctx, name, [&](tsn_atom name) -> int { return tsn_set_class_static_property_setter(ctx, cls, name, setter); });
}

tsn_value tsn_get_func_args_object(tsn_vm* ctx, int argc, tsn_value_const* argv, int start_index) {
    auto output = tsn_new_array(ctx);
    if (tsn_is_exception(ctx, output)) {
        return tsn_exception(ctx);
    }

    for (int i = start_index; i < argc; i++) {
        if (tsn_set_property_index(ctx, output, static_cast<uint32_t>(i), argv[i]) < 0) {
            tsn_release(ctx, output);
            return tsn_exception(ctx);
        }
    }

    return output;
}

void tsn_assign_module_atoms(tsn_vm* ctx, tsn_module* module, const tsn_string_def* atoms, size_t num_atoms) {
    for (size_t i = 0; i < num_atoms; ++i) {
        module->atoms[i] = tsn_create_atom_len(ctx, atoms[i].str, atoms[i].size);
    }
}

void tsn_assign_module_strings(tsn_vm* ctx, tsn_module* module, const tsn_string_def* strings, size_t num_strings) {
    for (size_t i = 0; i < num_strings; ++i) {
        tsn_set_module_const_string(ctx, module, i, strings[i].str, strings[i].size);
    }
}

tsn_atom tsn_create_atom_len(tsn_vm* ctx, const char* str, size_t len) {
    return JS_NewAtomLen(ctx, str, len);
}

tsn_var_ref tsn_get_closure_var(tsn_vm* ctx, tsn_closure* closure, int index) {
    return tsn_retain_var_ref(ctx, closure->var_refs[index]);
}

tsn_value tsn_get_func_arg(tsn_vm* ctx, int argc, tsn_value_const* argv, int index) {
    return index >= argc ? tsn_undefined(ctx) : JS_DupValue(ctx, argv[index]);
}

tsn_value tsn_get_global(tsn_vm* ctx) {
    return JS_GetGlobalObject(ctx);
}

tsn_value tsn_get_module_const(tsn_vm* ctx, tsn_module* module, size_t index) {
    return tsn_retain(ctx, module->constants[index]);
}

tsn_result tsn_set_module_const_string(
    tsn_vm* ctx, tsn_module* module, size_t index, const char* str, size_t str_length) {
    auto value = tsn_new_string_with_len(ctx, str, str_length);
    if (tsn_is_exception(ctx, value)) {
        return tsn_result_failure;
    }
    module->constants[index] = value;
    return tsn_result_success;
}

tsn_value tsn_get_property(
    tsn_vm* ctx, const tsn_value_const* obj, tsn_atom prop, const tsn_value_const* this_obj, tsn_prop_cache* ic) {
    return JS_GetProperty_tsn(ctx, *obj, prop, *this_obj, ic);
}

tsn_result tsn_get_property_free(tsn_vm* ctx,
                                 tsn_value* variable,
                                 const tsn_value_const* obj,
                                 tsn_atom prop,
                                 const tsn_value_const* this_obj,
                                 tsn_prop_cache* ic) {
    JS_FreeValue(ctx, *variable);
    *variable = JS_GetProperty_tsn(ctx, *obj, prop, *this_obj, ic);
    return tsn_is_exception(ctx, *variable) ? tsn_result_failure : tsn_result_success;
}

tsn_value tsn_get_property_index(tsn_vm* ctx, tsn_value_const this_obj, uint32_t index) {
    return JS_GetPropertyUint32(ctx, this_obj, index);
}

tsn_value tsn_get_property_value(tsn_vm* ctx, tsn_value_const obj, tsn_value_const value, tsn_value_const this_obj) {
    return JS_GetPropertyValue_tsn(ctx, obj, JS_DupValue(ctx, value), this_obj);
}

tsn_result tsn_set_property_str(tsn_vm* ctx, tsn_value m, const char* export_name, tsn_value val) {
    return tsn_process_result(ctx, JS_SetPropertyStr(ctx, m, export_name, JS_DupValue(ctx, val)));
}

tsn_result tsn_set_property(tsn_vm* ctx, const tsn_value* m, tsn_atom export_name, const tsn_value* val) {
    return tsn_process_result(ctx, JS_SetProperty_tsn(ctx, *m, export_name, JS_DupValue(ctx, *val)));
}

tsn_result tsn_set_properties(
    tsn_vm* ctx, const tsn_value* m, int nProps, const int* atoms, const tsn_value* values, const tsn_module* module) {
    tsn_result ret = tsn_result_success;
    for (int i = 0; i < nProps; ++i) {
        ret = tsn_process_result(ctx, JS_SetProperty(ctx, *m, module->atoms[atoms[i]], JS_DupValue(ctx, values[i])));
        if (ret != tsn_result_success) {
            return ret;
        }
    }
    return ret;
}

tsn_result tsn_set_property_value(tsn_vm* ctx, tsn_value this_obj, tsn_value property, tsn_value val) {
    return tsn_process_result(
        ctx, JS_SetPropertyValue_tsn(ctx, this_obj, JS_DupValue(ctx, property), JS_DupValue(ctx, val)));
}

tsn_result tsn_set_property_index(tsn_vm* ctx, tsn_value m, uint32_t index, tsn_value val) {
    return tsn_process_result(ctx, JS_SetPropertyUint32(ctx, m, index, JS_DupValue(ctx, val)));
}

tsn_value tsn_new_object(tsn_vm* ctx) {
    return JS_NewObject(ctx);
}

tsn_value tsn_new_array(tsn_vm* ctx) {
    return JS_NewArray(ctx);
}

tsn_value tsn_new_string_with_len(tsn_vm* ctx, const char* str1, size_t len1) {
    return JS_NewStringLen(ctx, str1, len1);
}

int tsn_execute_pending_jobs(tsn_runtime* rt) {
    JSContext* context = nullptr;

    for (;;) {
        switch (JS_ExecutePendingJob(rt, &context)) {
            case 0: // no pending job
                break;
            case 1: // job executed
                continue;
            case -1: // exception
                return -1;
        }
        auto remaining_timers = tsn_handle_timers();
        if (JS_IsJobPending(rt)) {
            // timer produced jobs
            continue;
        }
        if (remaining_timers > 0) {
            // more timers but no pending job, wait until next timer
            std::this_thread::sleep_until(tsn_next_timer());
        }
        // no more timer and no pending job
        break;
    }

    return exitCode;
}

tsn_value tsn_retain(tsn_vm* ctx, tsn_value_const v) {
    return JS_DupValue(ctx, v);
}

void tsn_release(tsn_vm* ctx, tsn_value v) {
    JS_FreeValue(ctx, v);
}

tsn_value tsn_retain_ptr(tsn_vm* ctx, const tsn_value_const* v) {
    return JS_DupValue(ctx, *v);
}

void tsn_release_ptr(tsn_vm* ctx, const tsn_value* v) {
    JS_FreeValue(ctx, *v);
}

tsn_value tsn_resume(tsn_vm* ctx, tsn_closure* closure, int argc, tsn_value_const* argv) {
    closure->resume_point = 0;
    if (tsn_has_exception(ctx)) {
        return JS_EXCEPTION;
    }
    return argc > 0 ? tsn_retain(ctx, argv[0]) : JS_UNDEFINED;
}

tsn_value tsn_math_log(tsn_vm* ctx, tsn_value op) {
    double d;
    if (JS_ToFloat64(ctx, &d, op) != 0) {
        return JS_EXCEPTION;
    }
    return tsn_double(ctx, std::log(d));
}

tsn_value tsn_math_pow(tsn_vm* ctx, tsn_value base, tsn_value exp) {
    double dbase;
    double dexp;
    if (JS_ToFloat64(ctx, &dbase, base) != 0) {
        return JS_EXCEPTION;
    }
    if (JS_ToFloat64(ctx, &dexp, exp) != 0) {
        return JS_EXCEPTION;
    }
    return tsn_double(ctx, std::pow(dbase, dexp));
}

tsn_value tsn_math_floor(tsn_vm* ctx, tsn_value op) {
    double d;
    if (unlikely(JS_ToFloat64(ctx, &d, op) != 0)) {
        return JS_EXCEPTION;
    }
    return tsn_int64(ctx, static_cast<int64_t>(std::floor(d)));
}

tsn_value tsn_math_max_v(tsn_vm* ctx, int n, tsn_value* operands) {
    assert(n > 0);
    if (n == 2 && likely(JS_VALUE_IS_BOTH_INT(operands[0], operands[1]))) {
        int x;
        int y;
        if (unlikely(JS_ToInt32(ctx, &x, operands[0]) != 0)) {
            return JS_EXCEPTION;
        }
        if (unlikely(JS_ToInt32(ctx, &y, operands[1]) != 0)) {
            return JS_EXCEPTION;
        }
        return x > y ? operands[0] : operands[1];
    }
    if (n == 2 && likely(JS_VALUE_IS_BOTH_FLOAT(operands[0], operands[1]))) {
        double x;
        double y;
        if (unlikely(JS_ToFloat64(ctx, &x, operands[0]) != 0)) {
            return JS_EXCEPTION;
        }
        if (unlikely(JS_ToFloat64(ctx, &y, operands[1]) != 0)) {
            return JS_EXCEPTION;
        }
        return x > y ? operands[0] : operands[1];
    }
    // more than 2 args or mixed types
    // maxVal <= [0]
    double maxVal;
    if (JS_ToFloat64(ctx, &maxVal, operands[0]) != 0) {
        return JS_EXCEPTION;
    }
    // for 1..n-1
    //   maxVal = max(max, [i])
    for (int i = 1; i < n; ++i) {
        double x;
        if (JS_ToFloat64(ctx, &x, operands[i]) != 0) {
            return JS_EXCEPTION;
        }
        if (x > maxVal) {
            maxVal = x;
        }
    }
    return tsn_double(ctx, maxVal);
}

// NOLINTEND(readability-identifier-naming)
