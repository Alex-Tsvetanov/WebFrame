//
//  SCValdiBuiltinBlockSupport.m
//  valdi-ios
//
//  Created by Simon Corsin on 6/3/20.
//

#import "valdi_core/SCValdiBuiltinTrampolines.h"

SCValdiFieldValue SCValdiFunctionInvokeO_v(const void *function, SCValdiFieldValue* fields)
{
    ((void(*)(const void *))function)(SCValdiFieldValueGetPtr(fields[0]));
    return SCValdiFieldValueMakeNull();
}

SCValdiFieldValue SCValdiFunctionInvokeOO_v(const void *function, SCValdiFieldValue* fields)
{
    ((void(*)(const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]));
    return SCValdiFieldValueMakeNull();
}

SCValdiFieldValue SCValdiFunctionInvokeOOO_v(const void *function, SCValdiFieldValue* fields)
{
    ((void(*)(const void *, const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]));
    return SCValdiFieldValueMakeNull();
}

SCValdiFieldValue SCValdiFunctionInvokeOOOO_v(const void *function, SCValdiFieldValue* fields)
{
    ((void(*)(const void *, const void *, const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]), SCValdiFieldValueGetPtr(fields[3]));
    return SCValdiFieldValueMakeNull();
}

SCValdiFieldValue SCValdiFunctionInvokeOOOOO_v(const void *function, SCValdiFieldValue* fields)
{
    ((void(*)(const void *, const void *, const void *, const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]), SCValdiFieldValueGetPtr(fields[3]), SCValdiFieldValueGetPtr(fields[4]));
    return SCValdiFieldValueMakeNull();
}

SCValdiFieldValue SCValdiFunctionInvokeOOOOOO_v(const void *function, SCValdiFieldValue* fields)
{
    ((void(*)(const void *, const void *, const void *, const void *, const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]), SCValdiFieldValueGetPtr(fields[3]), SCValdiFieldValueGetPtr(fields[4]), SCValdiFieldValueGetPtr(fields[5]));
    return SCValdiFieldValueMakeNull();
}

SCValdiFieldValue SCValdiFunctionInvokeO_o(const void *function, SCValdiFieldValue* fields)
{
    const void *result = ((const void *(*)(const void *))function)(SCValdiFieldValueGetPtr(fields[0]));
    return SCValdiFieldValueMakePtr(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOO_o(const void *function, SCValdiFieldValue* fields)
{
    const void *result = ((const void *(*)(const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]));
    return SCValdiFieldValueMakePtr(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOOO_o(const void *function, SCValdiFieldValue* fields)
{
    const void *result = ((const void *(*)(const void *, const void *, const void *))function)(
                                                                                               SCValdiFieldValueGetPtr(fields[0]),
                                                                                               SCValdiFieldValueGetPtr(fields[1]),
                                                                                               SCValdiFieldValueGetPtr(fields[2])
                                                                                               );
    return SCValdiFieldValueMakePtr(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOOOO_o(const void *function, SCValdiFieldValue* fields)
{
    const void *result = ((const void *(*)(const void *, const void *, const void *, const void *))function)(
                                                                                                             SCValdiFieldValueGetPtr(fields[0]),
                                                                                                             SCValdiFieldValueGetPtr(fields[1]),
                                                                                                             SCValdiFieldValueGetPtr(fields[2]),
                                                                                                             SCValdiFieldValueGetPtr(fields[3])
                                                                                               );
    return SCValdiFieldValueMakePtr(result);
}

SCValdiFieldValue SCValdiFunctionInvokeO_b(const void *function, SCValdiFieldValue* fields)
{
    BOOL result = ((BOOL(*)(const void *))function)(SCValdiFieldValueGetPtr(fields[0]));
    return SCValdiFieldValueMakeBool(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOO_b(const void *function, SCValdiFieldValue* fields)
{
    BOOL result = ((BOOL(*)(const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]));
    return SCValdiFieldValueMakeBool(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOOO_b(const void *function, SCValdiFieldValue* fields)
{
    BOOL result = ((BOOL(*)(const void *, const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]));
    return SCValdiFieldValueMakeBool(result);
}

SCValdiFieldValue SCValdiFunctionInvokeO_d(const void *function, SCValdiFieldValue* fields)
{
    double result = ((double(*)(const void *))function)(SCValdiFieldValueGetPtr(fields[0]));
    return SCValdiFieldValueMakeDouble(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOO_d(const void *function, SCValdiFieldValue* fields)
{
    double result = ((double(*)(const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]));
    return SCValdiFieldValueMakeDouble(result);
}

SCValdiFieldValue SCValdiFunctionInvokeOOO_d(const void *function, SCValdiFieldValue* fields)
{
    double result = ((double(*)(const void *, const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]));
    return SCValdiFieldValueMakeDouble(result);
}

id SCValdiBlockCreateO_v(SCValdiBlockTrampoline trampoline)
{
    return ^{
        trampoline(nil);
    };
}

id SCValdiBlockCreateOO_v(SCValdiBlockTrampoline trampoline)
{
    return ^(const void *p0) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0));
    };
}

id SCValdiBlockCreateOOO_v(SCValdiBlockTrampoline trampoline)
{
    return ^(const void *p0, const void *p1) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1));
    };
}

id SCValdiBlockCreateOOOO_v(SCValdiBlockTrampoline trampoline)
{
    return ^(const void *p0, const void *p1, const void *p2) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1), SCValdiFieldValueMakePtr(p2));
    };
}

id SCValdiBlockCreateOOOOO_v(SCValdiBlockTrampoline trampoline)
{
    return ^(const void *p0, const void *p1, const void *p2, const void *p3) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1), SCValdiFieldValueMakePtr(p2), SCValdiFieldValueMakePtr(p3));
    };
}

id SCValdiBlockCreateOOOOOO_v(SCValdiBlockTrampoline trampoline)
{
    return ^(const void *p0, const void *p1, const void *p2, const void *p3, const void *p4) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1), SCValdiFieldValueMakePtr(p2), SCValdiFieldValueMakePtr(p3), SCValdiFieldValueMakePtr(p4));
    };
}

id SCValdiBlockCreateO_o(SCValdiBlockTrampoline trampoline)
{
    return ^const void *() {
        return SCValdiFieldValueGetPtr(trampoline(nil));
    };
}

id SCValdiBlockCreateOO_o(SCValdiBlockTrampoline trampoline)
{
    return ^const void *(const void *p0) {
        return SCValdiFieldValueGetPtr(trampoline(nil, SCValdiFieldValueMakePtr(p0)));
    };
}

id SCValdiBlockCreateOOO_o(SCValdiBlockTrampoline trampoline)
{
    return ^const void *(const void *p0, const void *p1) {
        return SCValdiFieldValueGetPtr(trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1)));
    };
}

id SCValdiBlockCreateOOOO_o(SCValdiBlockTrampoline trampoline)
{
    return ^const void *(const void *p0, const void *p1, const void *p2) {
        return SCValdiFieldValueGetPtr(trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1), SCValdiFieldValueMakePtr(p2)));
    };
}

id SCValdiBlockCreateO_d(SCValdiBlockTrampoline trampoline)
{
    return ^double() {
        return SCValdiFieldValueGetDouble(trampoline(nil));
    };
}

id SCValdiBlockCreateOO_d(SCValdiBlockTrampoline trampoline)
{
    return ^double(const void *p0) {
        return SCValdiFieldValueGetDouble(trampoline(nil, SCValdiFieldValueMakePtr(p0)));
    };
}

id SCValdiBlockCreateOOO_d(SCValdiBlockTrampoline trampoline)
{
    return ^double(const void *p0, const void *p1) {
        return SCValdiFieldValueGetDouble(trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1)));
    };
}

id SCValdiBlockCreateO_b(SCValdiBlockTrampoline trampoline)
{
    return ^BOOL() {
        return SCValdiFieldValueGetBool(trampoline(nil));
    };
}

id SCValdiBlockCreateOO_b(SCValdiBlockTrampoline trampoline)
{
    return ^BOOL(const void *p0) {
        return SCValdiFieldValueGetBool(trampoline(nil, SCValdiFieldValueMakePtr(p0)));
    };
}

id SCValdiBlockCreateOOO_b(SCValdiBlockTrampoline trampoline)
{
    return ^BOOL(const void *p0, const void *p1) {
        return SCValdiFieldValueGetBool(trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1)));
    };
}
