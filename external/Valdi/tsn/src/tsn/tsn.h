#pragma once

#include "quickjs/quickjs.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define js_force_inline static inline __attribute__((always_inline, unused))

#define TSN_EVAL_TYPE_GLOBAL JS_EVAL_TYPE_GLOBAL
#define TSN_EVAL_TYPE_MODULE JS_EVAL_TYPE_MODULE
#define TSN_EVAL_FLAG_STRICT JS_EVAL_FLAG_STRICT

typedef JSValue tsn_value;
typedef JSValueConst tsn_value_const;
typedef JSContext tsn_vm;
typedef JSRuntime tsn_runtime;
typedef JSAtom tsn_atom;
typedef JS_BOOL tsn_bool;

typedef JSStackFrame tsn_stackframe;

typedef tsn_value (*tsn_module_init_fn)(tsn_vm* ctx);

typedef JS_PropCache_tsn tsn_prop_cache;

typedef struct {
    const char* name;
    uint32_t atoms_length;
    uint32_t constants_length;
    tsn_atom* atoms;
    tsn_value* constants;
    uint32_t prop_cache_slots;
    tsn_prop_cache* prop_cache;
    tsn_value module_as_value;
} tsn_module;

struct tsn_closure;
typedef struct tsn_closure tsn_closure;

typedef tsn_value tsn_generic_func(tsn_vm* ctx,
                                   tsn_value_const this_val,
                                   tsn_stackframe* stackframe,
                                   int argc,
                                   tsn_value_const* argv,
                                   tsn_closure* closure);

typedef JSVarRef* tsn_var_ref;

typedef int tsn_result;

#define tsn_result_success 0
#define tsn_result_failure -1

struct tsn_closure {
    tsn_module* module;
    tsn_generic_func* callable;
    bool is_class_constructor;
    uint32_t ref_count;
    int32_t resume_point;     /* for generators and async functions. -1 means completed */
    uint32_t stack_vrefs;     /* local variable refs stored with the closure */
    uint32_t stack_values;    /* local values stored with the closure */
    uint32_t stack_iterators; /* local iterators stored with the closure */
    uint32_t var_refs_length; /* variable refs captured from parent functions */
    tsn_var_ref var_refs[0];
};

typedef struct tsn_iterator {
    tsn_value iterator_value;
    tsn_value iterator_method;
    bool has_dequeued_value;
} tsn_iterator;

js_force_inline void* get_closure_stack(tsn_closure* closure) {
    return (uint8_t*)closure + sizeof(tsn_closure) + sizeof(tsn_var_ref) * closure->var_refs_length;
}

// Copied from QuickJS internal. Struct needs to match

struct list_head {
    struct list_head* prev;
    struct list_head* next;
};

typedef struct JSStackFrame {
    struct JSStackFrame* prev_frame; /* NULL if first stack frame */
    JSValue cur_func;                /* current function, JS_UNDEFINED if the frame is detached */
    JSValue* arg_buf;                /* arguments */
    JSValue* var_buf;                /* variables */
    struct list_head var_ref_list;   /* list of JSVarRef.var_ref_link */
    const uint8_t* cur_pc;           /* only used in bytecode functions : PC of the
                                      instruction after the call */
    int arg_count;
    int js_mode; /* for C functions, only JS_MODE_MATH may be set */
    /* only used in generators. Current stack pointer value. NULL if
     the function is running. */
    JSValue* cur_sp;
} JSStackFrame;

js_force_inline tsn_runtime* tsn_get_runtime(tsn_vm* ctx) {
    return JS_GetRuntime(ctx);
}

tsn_module* tsn_new_module(tsn_vm* ctx, size_t atoms_length, size_t constants_length, size_t prop_cache_slots);
void __tsn_deallocate_module(tsn_runtime* rt, tsn_module* module);

void tsn_retain_module(tsn_vm* ctx, tsn_module* module);

void tsn_free_module(tsn_vm* ctx, tsn_module* module);
void tsn_free_module_rt(tsn_runtime* rt, tsn_module* module);

tsn_value tsn_new_module_init_fn(tsn_vm* ctx, tsn_module* module, tsn_generic_func* func);

void __tsn_deallocate_closure(tsn_runtime* rt, tsn_closure* closure);

js_force_inline void tsn_retain_closure(tsn_runtime* rt, tsn_closure* closure) {
    ++closure->ref_count;
}

js_force_inline void tsn_free_closure(tsn_runtime* rt, tsn_closure* closure) {
    if (--closure->ref_count == 0) {
        __tsn_deallocate_closure(rt, closure);
    }
}

tsn_value tsn_retain(tsn_vm* ctx, tsn_value_const v);
void tsn_release(tsn_vm* ctx, tsn_value v);
tsn_value tsn_retain_ptr(tsn_vm* ctx, const tsn_value_const* v);
void tsn_release_ptr(tsn_vm* ctx, const tsn_value* v);
void tsn_release_vars(tsn_vm* ctx, int n, tsn_value** vars);
void tsn_release_var_refs(tsn_vm* ctx, int n, tsn_var_ref* var_refs);
void tsn_release_iterators(tsn_vm* ctx, int n, tsn_iterator** iterators);
js_force_inline tsn_value tsn_retain_inline(tsn_vm* ctx, tsn_value_const v) {
    return JS_DupValue(ctx, v);
}
js_force_inline void tsn_release_inline(tsn_vm* ctx, tsn_value v) {
    JS_FreeValue(ctx, v);
}

typedef struct {
    const char* str;
    size_t size;
} tsn_string_def;

void tsn_assign_module_atoms(tsn_vm* ctx, tsn_module* module, const tsn_string_def* atoms, size_t num_atoms);
void tsn_assign_module_strings(tsn_vm* ctx, tsn_module* module, const tsn_string_def* strings, size_t num_strings);

tsn_atom tsn_create_atom_len(tsn_vm* ctx, const char* str, size_t len);

js_force_inline void tsn_free_atom(tsn_vm* ctx, tsn_atom v) {
    JS_FreeAtom(ctx, v);
}

js_force_inline tsn_value tsn_double(tsn_vm* ctx, double d) {
    return JS_NewFloat64(ctx, d);
}

js_force_inline tsn_value tsn_int32(tsn_vm* ctx, int32_t i) {
    return JS_NewInt32(ctx, i);
}

js_force_inline tsn_value tsn_int64(tsn_vm* ctx, int64_t i) {
    return JS_NewInt64(ctx, i);
}

js_force_inline tsn_value tsn_new_bool(tsn_vm* ctx, bool val) {
    return JS_NewBool(ctx, val);
}

js_force_inline bool tsn_to_bool(tsn_vm* ctx, tsn_value_const val) {
    return JS_ToBool(ctx, val);
}

js_force_inline bool tsn_truthy_bool(tsn_vm* ctx, tsn_value_const val) {
    return JS_VALUE_GET_BOOL(val);
}

js_force_inline bool tsn_not_undefined_or_null(tsn_vm* ctx, tsn_value_const val) {
    return !(JS_IsUndefined(val) || JS_IsNull(val));
}
js_force_inline bool tsn_is_undefined_or_null(tsn_vm* ctx, tsn_value_const val) {
    return JS_IsUndefined(val) || JS_IsNull(val);
}
js_force_inline bool tsn_is_undefined(tsn_vm* ctx, tsn_value_const val) {
    return JS_IsUndefined(val) != 0;
}
js_force_inline bool tsn_not_undefined(tsn_vm* ctx, tsn_value_const val) {
    return JS_IsUndefined(val) == 0;
}
js_force_inline bool tsn_is_null(tsn_vm* ctx, tsn_value_const val) {
    return JS_IsNull(val) != 0;
}
js_force_inline bool tsn_not_null(tsn_vm* ctx, tsn_value_const val) {
    return JS_IsNull(val) == 0;
}

tsn_value tsn_get_module_const(tsn_vm* ctx, tsn_module* module, size_t index);
tsn_result tsn_set_module_const_string(
    tsn_vm* ctx, tsn_module* module, size_t index, const char* str, size_t str_length);

tsn_result tsn_set_property_str(tsn_vm* ctx, tsn_value m, const char* export_name, tsn_value val);

tsn_result tsn_set_property(tsn_vm* ctx, const tsn_value* m, tsn_atom export_name, const tsn_value* val);

tsn_result tsn_set_properties(
    tsn_vm* ctx, const tsn_value* m, int nProps, const int* atoms, const tsn_value* values, const tsn_module* module);

tsn_result tsn_set_property_value(tsn_vm* ctx, tsn_value this_obj, tsn_value property, tsn_value val);

tsn_result tsn_set_property_index(tsn_vm* ctx, tsn_value m, uint32_t index, tsn_value val);

js_force_inline tsn_value tsn_undefined(tsn_vm* ctx) {
    return JS_UNDEFINED;
}

js_force_inline tsn_value tsn_null(tsn_vm* ctx) {
    return JS_NULL;
}

js_force_inline tsn_value tsn_exception(tsn_vm* ctx) {
    return JS_EXCEPTION;
}

js_force_inline bool tsn_is_exception(tsn_vm* ctx, tsn_value_const v) {
    return JS_IsException(v) == 1;
}

bool tsn_has_exception(tsn_vm* ctx);

js_force_inline bool tsn_iterator_is_exception(tsn_vm* ctx, tsn_iterator* iterator) {
    return tsn_is_exception(ctx, iterator->iterator_value);
}

js_force_inline bool tsn_var_ref_is_exception(tsn_vm* vm, tsn_var_ref v) {
    return v == NULL;
}

tsn_value tsn_op_div(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_mult(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_add(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_sub(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_neg(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_plus(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_inc(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_dec(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_bnot(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_lnot(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_typeof(tsn_vm* ctx, tsn_value op1);
tsn_value tsn_op_lt(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_lte(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_lte_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_gt(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_gte(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_gte_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_eq(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_eq_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_ne(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_ne_strict(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_exp(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_mod(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_instanceof(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_in(tsn_vm* ctx, tsn_value op1, tsn_value op2);

tsn_value tsn_op_ls(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_rs(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_urs(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_bxor(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_band(tsn_vm* ctx, tsn_value op1, tsn_value op2);
tsn_value tsn_op_bor(tsn_vm* ctx, tsn_value op1, tsn_value op2);

void tsn_throw(tsn_vm* ctx, tsn_value exception);
tsn_value tsn_get_exception(tsn_vm* ctx);

int tsn_get_ref_count(tsn_vm* ctx, tsn_value_const v);

tsn_var_ref tsn_new_var_ref(tsn_vm* ctx);
tsn_value tsn_load_var_ref(tsn_vm* ctx, tsn_var_ref var_ref);
void tsn_set_var_ref(tsn_vm* ctx, tsn_var_ref var_ref, tsn_value value);
tsn_var_ref tsn_retain_var_ref(tsn_vm* ctx, tsn_var_ref var_ref);
tsn_var_ref tsn_release_var_ref(tsn_vm* ctx, tsn_var_ref var_ref);
void tsn_release_var_ref_rt(tsn_runtime* rt, tsn_var_ref var_ref);
js_force_inline tsn_var_ref tsn_empty_var_ref(tsn_vm* ctx) {
    return NULL;
}

tsn_iterator tsn_empty_iterator(tsn_vm* ctx);
tsn_iterator tsn_new_iterator(tsn_vm* ctx, tsn_value iterable);

tsn_value tsn_new_generator(tsn_vm* ctx, tsn_value closure);

js_force_inline bool tsn_iterator_has_value(tsn_vm* ctx, tsn_iterator* iterator) {
    return iterator->has_dequeued_value;
}

tsn_iterator tsn_new_keys_iterator(tsn_vm* ctx, tsn_value iterable);
tsn_iterator tsn_retain_iterator(tsn_vm* ctx, tsn_iterator* iterator);
tsn_iterator tsn_release_iterator(tsn_vm* ctx, tsn_iterator* iterator);
tsn_value tsn_iterator_next(tsn_vm* ctx, tsn_iterator* iterator);
tsn_value tsn_keys_iterator_next(tsn_vm* ctx, tsn_iterator* iterator);

tsn_value tsn_get_global(tsn_vm* ctx);

tsn_value tsn_new_object(tsn_vm* ctx);

tsn_value tsn_new_array(tsn_vm* ctx);

tsn_value tsn_new_arrow_function(tsn_vm* ctx,
                                 tsn_module* module,
                                 tsn_generic_func* func,
                                 int32_t argc,
                                 uint32_t stack_vrefs,
                                 uint32_t stack_values,
                                 uint32_t stack_iterators);

tsn_value tsn_new_arrow_function_closure(tsn_vm* ctx,
                                         tsn_module* module,
                                         tsn_generic_func* func,
                                         int32_t argc,
                                         size_t vars_length,
                                         tsn_var_ref* var,
                                         uint32_t stack_vrefs,
                                         uint32_t stack_values,
                                         uint32_t stack_iterators);

tsn_value tsn_new_function(tsn_vm* ctx, tsn_module* module, tsn_atom name, tsn_generic_func* func, int32_t argc);

tsn_value tsn_new_function_closure(tsn_vm* ctx,
                                   tsn_module* module,
                                   tsn_atom name,
                                   tsn_generic_func* func,
                                   int32_t argc,
                                   size_t vars_length,
                                   tsn_var_ref* vars);

tsn_value tsn_new_class(
    tsn_vm* ctx, tsn_module* module, tsn_atom name, tsn_generic_func* func, int32_t argc, tsn_value_const parent_ctor);

tsn_value tsn_new_class_closure(tsn_vm* ctx,
                                tsn_module* module,
                                tsn_atom name,
                                tsn_generic_func* func,
                                int32_t argc,
                                tsn_value_const parent_ctor,
                                size_t vars_length,
                                tsn_var_ref* vars);

tsn_result tsn_set_class_method(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value method);
tsn_result tsn_set_class_method_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value method);

tsn_result tsn_set_class_static_method(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value method);
tsn_result tsn_set_class_static_method_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value method);

tsn_result tsn_set_class_property_getter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value getter);
tsn_result tsn_set_class_property_getter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value getter);

tsn_result tsn_set_class_static_property_getter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value getter);
tsn_result tsn_set_class_static_property_getter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value getter);

tsn_result tsn_set_class_property_setter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value setter);
tsn_result tsn_set_class_property_setter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value setter);

tsn_result tsn_set_class_static_property_setter(tsn_vm* ctx, tsn_value cls, tsn_atom name, tsn_value setter);
tsn_result tsn_set_class_static_property_setter_value(tsn_vm* ctx, tsn_value cls, tsn_value name, tsn_value setter);

tsn_value tsn_get_func_arg(tsn_vm* ctx, int argc, tsn_value_const* argv, int index);

tsn_value tsn_get_func_args_object(tsn_vm* ctx, int argc, tsn_value_const* argv, int start_index);

tsn_var_ref tsn_get_closure_var(tsn_vm* ctx, tsn_closure* closure, int index);

tsn_value tsn_new_string_with_len(tsn_vm* ctx, const char* str1, size_t len1);

tsn_value tsn_get_property_str(tsn_vm* ctx, tsn_value_const this_obj, const char* prop);

// get without ic
tsn_value tsn_get_property0(tsn_vm* ctx, const tsn_value_const* obj, tsn_atom prop);
// get with ic
tsn_value tsn_get_property(
    tsn_vm* ctx, const tsn_value_const* obj, tsn_atom prop, const tsn_value_const* this_obj, tsn_prop_cache* ic);
tsn_result tsn_get_property_free(tsn_vm* ctx,
                                 tsn_value* variable,
                                 const tsn_value_const* obj,
                                 tsn_atom prop,
                                 const tsn_value_const* this_obj,
                                 tsn_prop_cache* ic);

tsn_value tsn_get_property_index(tsn_vm* ctx, tsn_value_const this_obj, uint32_t index);

tsn_value tsn_get_property_value(tsn_vm* ctx, tsn_value_const obj, tsn_value_const value, tsn_value_const this_obj);

tsn_value tsn_delete_property(tsn_vm* ctx, tsn_value_const obj, tsn_atom prop);
tsn_value tsn_delete_property_value(tsn_vm* ctx, tsn_value_const obj, tsn_value_const prop);

tsn_value tsn_copy_properties(tsn_vm* ctx, tsn_value_const dest_obj, tsn_value_const src_obj);
tsn_value tsn_copy_filtered_properties(
    tsn_vm* ctx, tsn_value_const dest_obj, tsn_value_const src_obj, int argc, tsn_atom* atoms_to_ignore);

tsn_value tsn_get_super(tsn_vm* ctx, const tsn_stackframe* stackframe);
tsn_value tsn_get_super_constructor(tsn_vm* ctx, const tsn_stackframe* stackframe);

tsn_value tsn_call(tsn_vm* ctx, tsn_value_const func_obj, tsn_value_const this_obj, int argc, tsn_value_const* argv);
tsn_value tsn_call_vargs(tsn_vm* ctx, tsn_value_const func_obj, tsn_value_const this_obj, tsn_value_const args);
tsn_value tsn_call_fargs(
    tsn_vm* ctx, tsn_value_const func_obj, tsn_value_const this_obj, int argc, tsn_value_const* argv);
tsn_value tsn_call_constructor(
    tsn_vm* ctx, tsn_value_const ctor, tsn_value_const new_target, int argc, tsn_value_const* argv);
tsn_value tsn_call_constructor_vargs(tsn_vm* ctx,
                                     tsn_value_const ctor,
                                     tsn_value_const new_target,
                                     tsn_value_const args);

/**
 * Registers a module into the registry before the application finished launching.
 * This should be only called when the native binary is being loaded and before the
 * main() function has started evaluating.
 */
void tsn_register_module(char const* module_name, tsn_module_init_fn module_init_fn);

/**
 * Registers a module into the registry before the application finished launching.
 * This should be only called when the native binary is being loaded and before the
 * main() function has started evaluating.
 */
void tsn_register_module_pre_launch(char const* module_name, const char* sha256, tsn_module_init_fn module_init_fn);

/**
 * Registers a module into the registry after the main() function has started evaluating.
 */
void tsn_register_module_post_launch(const char* module_name,
                                     const char* sha256,
                                     const tsn_module_init_fn module_init_fn);

tsn_vm* tsn_init(int argc, const char** argv);
tsn_value tsn_load_module(tsn_vm* ctx, const char* module_name);

tsn_value tsn_require(tsn_vm* ctx, tsn_atom module_name);
tsn_value tsn_require_from_string(tsn_vm* ctx, const char* module_name);

void tsn_load_in_context(JSContext* context);
void tsn_unload_in_context(JSContext* context);

js_force_inline tsn_value
tsn_eval(tsn_vm* ctx, const char* input, size_t input_len, const char* filename, int eval_flags) {
    return JS_Eval(ctx, input, input_len, filename, eval_flags);
}
void tsn_fini(tsn_vm** vm);

void tsn_dump_obj(tsn_vm* ctx, FILE* f, tsn_value val);

void tsn_debug(tsn_vm* ctx, tsn_value_const value, const char* expression);

void tsn_dump_refcount(tsn_value_const val, char const* name);

js_force_inline bool tsn_is_error(tsn_vm* ctx, tsn_value_const val) {
    return JS_IsError(ctx, val) != 0;
}

js_force_inline JSValue tsn_get_error_type(JSContext* ctx, tsn_value_const this_val) {
    return JS_GetErrorType(ctx, this_val);
}

js_force_inline JSValue tsn_get_error_description(JSContext* ctx, tsn_value_const this_val) {
    return JS_GetErrorDescription(ctx, this_val);
}

js_force_inline const char* tsn_to_cstring(tsn_vm* ctx, tsn_value_const val1) {
    return JS_ToCString(ctx, val1);
}

int tsn_execute_pending_jobs(tsn_runtime* rt);

typedef struct {
    const char* name;
    const char* sha256;
    tsn_module_init_fn module_init_fn;
} tsn_module_info;

bool tsn_contains_module(const char* module_name);
bool tsn_get_module_info(const char* module_name, tsn_module_info* output);

void tsn_dump_error(tsn_vm* ctx);

js_force_inline void tsn_set_pc(tsn_stackframe* stackframe, int lineNumber, int columnNumber) {
    // We use the arg_count to store the current line number
    stackframe->arg_count = lineNumber;
}

tsn_value tsn_resume(tsn_vm* ctx, tsn_closure* closure, int argc, tsn_value_const* argv);

tsn_value tsn_math_log(tsn_vm* ctx, tsn_value op);
tsn_value tsn_math_pow(tsn_vm* ctx, tsn_value base, tsn_value exp);
tsn_value tsn_math_floor(tsn_vm* ctx, tsn_value op);
tsn_value tsn_math_max_v(tsn_vm* ctx, int n, tsn_value* operands);

#ifdef __cplusplus
}
#endif
