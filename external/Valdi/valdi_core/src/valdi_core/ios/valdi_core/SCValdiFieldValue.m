//
//  SCValdiFieldValue.m
//  valdi-ios
//
//  Created by Simon Corsin on 6/3/20.
//

#import "valdi_core/SCValdiFieldValue.h"

id SCValdiFieldValueGetObject(SCValdiFieldValue fieldValue)
{
    return (__bridge id)fieldValue.o;
}

double SCValdiFieldValueGetDouble(SCValdiFieldValue fieldValue)
{
    return fieldValue.d;
}

BOOL SCValdiFieldValueGetBool(SCValdiFieldValue fieldValue)
{
    return fieldValue.b;
}

const void* SCValdiFieldValueGetPtr(SCValdiFieldValue fieldValue)
{
    return fieldValue.o;
}

int32_t SCValdiFieldValueGetInt(SCValdiFieldValue fieldValue)
{
    return fieldValue.i;
}

int64_t SCValdiFieldValueGetLong(SCValdiFieldValue fieldValue)
{
    return fieldValue.l;
}

SCValdiFieldValueDescriptor SCValdiFieldValueDescriptorMakeEmpty()
{
    SCValdiFieldValueDescriptor descriptor;
    descriptor.type = 0;
    descriptor.isOptional = false;
    descriptor.isCopyable = false;
    descriptor.isFunction = false;
    descriptor.selector = nil;
    return descriptor;
}

SCValdiFieldValue SCValdiFieldValueMakeObject(__unsafe_unretained id object)
{
    SCValdiFieldValue v;

    if (object) {
        v.o = CFBridgingRetain(object);
        CFAutorelease(v.o);
    } else {
        v.o = nil;
    }

    return v;
}

SCValdiFieldValue SCValdiFieldValueMakeUnretainedObject(__unsafe_unretained id object)
{
    SCValdiFieldValue v;
    v.o = (__bridge const void *)(object);
    return v;
}

SCValdiFieldValue SCValdiFieldValueMakeDouble(double d)
{
    SCValdiFieldValue v;
    v.d = d;
    return v;
}

SCValdiFieldValue SCValdiFieldValueMakeBool(BOOL b)
{
    SCValdiFieldValue v;
    v.b = b;
    return v;
}

SCValdiFieldValue SCValdiFieldValueMakePtr(const void* ptr)
{
    SCValdiFieldValue v;
    v.o = ptr;
    return v;
}

SCValdiFieldValue SCValdiFieldValueMakeInt(int32_t i)
{
    SCValdiFieldValue v;
    v.i = i;
    return v;
}

SCValdiFieldValue SCValdiFieldValueMakeLong(int64_t l)
{
    SCValdiFieldValue v;
    v.l = l;
    return v;
}

SCValdiFieldValue SCValdiFieldValueMakeNull()
{
    SCValdiFieldValue v;
    v.o = nil;
    return v;
}
