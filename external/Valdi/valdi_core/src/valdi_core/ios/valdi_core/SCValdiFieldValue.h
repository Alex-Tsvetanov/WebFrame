//
//  SCValdiFieldValue.h
//  valdi-ios
//
//  Created by Simon Corsin on 6/3/20.
//

#import "valdi_core/SCMacros.h"
#import <Foundation/NSObjCRuntime.h>
#import <Foundation/NSObject.h>

SC_EXTERN_C_BEGIN

typedef union SCValdiFieldValue {
    const void* o; // an NSObject or a raw ptr
    double d;
    BOOL b;
    int32_t i;
    int64_t l;
} SCValdiFieldValue;

extern id SCValdiFieldValueGetObject(SCValdiFieldValue fieldValue);
extern double SCValdiFieldValueGetDouble(SCValdiFieldValue fieldValue);
extern BOOL SCValdiFieldValueGetBool(SCValdiFieldValue fieldValue);
extern const void* SCValdiFieldValueGetPtr(SCValdiFieldValue fieldValue);
extern int32_t SCValdiFieldValueGetInt(SCValdiFieldValue fieldValue);
extern int64_t SCValdiFieldValueGetLong(SCValdiFieldValue fieldValue);

extern SCValdiFieldValue SCValdiFieldValueMakeObject(__unsafe_unretained id object);
extern SCValdiFieldValue SCValdiFieldValueMakeUnretainedObject(__unsafe_unretained id object);
extern SCValdiFieldValue SCValdiFieldValueMakeDouble(double d);
extern SCValdiFieldValue SCValdiFieldValueMakeBool(BOOL b);
extern SCValdiFieldValue SCValdiFieldValueMakeInt(int32_t i);
extern SCValdiFieldValue SCValdiFieldValueMakeLong(int64_t l);
extern SCValdiFieldValue SCValdiFieldValueMakePtr(const void* ptr);
extern SCValdiFieldValue SCValdiFieldValueMakeNull();

typedef enum : uint8_t {
    SCValdiFieldValueTypeVoid,
    SCValdiFieldValueTypePtr,
    SCValdiFieldValueTypeObject,
    SCValdiFieldValueTypeDouble,
    SCValdiFieldValueTypeBool,
    SCValdiFieldValueTypeInt,
    SCValdiFieldValueTypeLong,
} SCValdiFieldValueType;

typedef struct SCValdiFieldValueDescriptor {
    SCValdiFieldValueType type;
    BOOL isOptional;
    BOOL isCopyable;
    BOOL isFunction;
    SEL selector;
} SCValdiFieldValueDescriptor;

extern SCValdiFieldValueDescriptor SCValdiFieldValueDescriptorMakeEmpty();

/**
 A block trampoline takes a vararg of SCValdiFieldValue, put them into a marshaller,
 call a backing function with marshalled parameters, and return the result unmarshalled.
 */
typedef SCValdiFieldValue (^SCValdiBlockTrampoline)(const void* receiver, ...);

/**
 A function invoker takes an opaque function, calls it using the given unmarshalled parameter values,
 and return the raw result (not marshalled).
 */
typedef SCValdiFieldValue (*SCValdiFunctionInvoker)(const void* function, SCValdiFieldValue* fields);

/**
 A block factory takes a block trampoline and creates a concrete a concrete block which forwards
 the parameters to the trampoline.
 */
typedef id (*SCValdiBlockFactory)(SCValdiBlockTrampoline trampoline);

SC_EXTERN_C_END
