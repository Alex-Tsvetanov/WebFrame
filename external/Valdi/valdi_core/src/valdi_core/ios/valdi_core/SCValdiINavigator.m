#import "valdi_core/SCValdiMarshallableObjectDescriptor.h"
#import "valdi_core/SCValdiJSConversionUtils.h"

#import "SCValdiINavigator.h"

static SCValdiFieldValue SCValdiFunctionInvokeOOOB_v(const void *function, SCValdiFieldValue* fields) {
    ((void (*) (const void *, const void *, const void *, BOOL ))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]), SCValdiFieldValueGetBool(fields[3]));
    return SCValdiFieldValueMakeNull();
}

static id SCValdiBlockCreateOOOB_v(SCValdiBlockTrampoline trampoline) {
    return ^void(const void * p0, const void * p1, BOOL p2) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1), SCValdiFieldValueMakeBool(p2));
    };
}

static SCValdiFieldValue SCValdiFunctionInvokeOOB_v(const void *function, SCValdiFieldValue* fields) {
    ((void (*) (const void *, const void *, BOOL ))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetBool(fields[2]));
    return SCValdiFieldValueMakeNull();
}

static id SCValdiBlockCreateOOB_v(SCValdiBlockTrampoline trampoline) {
    return ^void(const void * p0, BOOL p1) {
        trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakeBool(p1));
    };
}

static SCValdiFieldValue SCValdiFunctionInvokeOD_v(const void *function, SCValdiFieldValue* fields) {
    ((void (*) (const void *, double ))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetDouble(fields[1]));
    return SCValdiFieldValueMakeNull();
}

static id SCValdiBlockCreateOD_v(SCValdiBlockTrampoline trampoline) {
    return ^void(double p0) {
        trampoline(nil, SCValdiFieldValueMakeDouble(p0));
    };
}

@interface SCValdiINavigator: SCValdiProxyMarshallableObject

@end

@implementation SCValdiINavigator

+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor
{
    static const SCValdiMarshallableObjectBlockSupport kBlocks[] = {
        { .typeEncoding = "ooob:v", .invoker = &SCValdiFunctionInvokeOOOB_v, .factory = &SCValdiBlockCreateOOOB_v },
        { .typeEncoding = "oob:v", .invoker = &SCValdiFunctionInvokeOOB_v, .factory = &SCValdiBlockCreateOOB_v },
        { .typeEncoding = "od:v", .invoker = &SCValdiFunctionInvokeOD_v, .factory = &SCValdiBlockCreateOD_v },
        { .typeEncoding = nil }
    };
    static const char *const kIdentifiers[] = {
        "SCValdiINavigatorPageConfig",
        nil
    };
    static const SCValdiMarshallableObjectFieldDescriptor kFields[] = {
        {.name = "pushComponent", .selName = "pushComponentWithPage:animated:", .type = "f*(r:'[0]', b)"},
        {.name = "pop", .selName = "popWithAnimated:", .type = "f*(b)"},
        {.name = "popToSelf", .selName = "popToSelfWithAnimated:", .type = "f*(b)"},
        {.name = "presentComponent", .selName = "presentComponentWithPage:animated:", .type = "f*(r:'[0]', b)"},
        {.name = "dismiss", .selName = "dismissWithAnimated:", .type = "f*(b)"},
        {.name = "forceDisableDismissalGesture", .selName = "forceDisableDismissalGestureWithForceDisable:", .type = "f*(b)"},
        {.name = "setBackButtonObserver", .selName = "setBackButtonObserverWithObserver:", .type = "f?*(f?())"},
        {.name = "setPageVisibilityObserver", .selName = "setPageVisibilityObserverWithObserver:", .type = "f?*(f?(d))"},
        {.name = nil},
    };
    return SCValdiMarshallableObjectDescriptorMake(kFields, kIdentifiers, kBlocks, SCValdiMarshallableObjectTypeProtocol);
}

@end

NSInteger SCValdiINavigatorMarshall(SCValdiMarshallerRef  _Nonnull marshaller, id<SCValdiINavigator> _Nonnull instance) {
    return SCValdiMarshallableObjectMarshallInterface(marshaller, instance, [SCValdiINavigator class]);
}
