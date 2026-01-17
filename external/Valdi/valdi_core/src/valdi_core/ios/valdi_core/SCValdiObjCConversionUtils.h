//
//  SCValdiObjCConversionUtils.h
//  valdi-ios
//
//  Created by Simon Corsin on 9/10/19.
//

#pragma once

#import <Foundation/Foundation.h>

#import "utils/base/NonCopyable.hpp"
#import "valdi_core/cpp/Utils/Marshaller.hpp"
#import "valdi_core/cpp/Utils/SequenceIDGenerator.hpp"
#import "valdi_core/cpp/Utils/StringBox.hpp"
#import "valdi_core/cpp/Utils/ValdiObject.hpp"
#import "valdi_core/cpp/Utils/Value.hpp"
#import "valdi_core/cpp/Utils/ValueConvertible.hpp"
#import "valdi_core/cpp/Utils/ValueTypedArray.hpp"

#import "valdi_core/SCValdiMarshaller.h"
#import "valdi_core/SCValdiValueUtils.h"

@protocol SCValdiFunction;

@interface SCValdiCppObject : NSObject

@property (nonatomic, readonly) Valdi::Ref<Valdi::RefCountable> ref;

+ (instancetype)objectWithRef:(Valdi::Ref<Valdi::RefCountable>)ref;

@end

namespace Valdi::IOS {
class ObjCReferenceTable;
}

namespace ValdiIOS {

/**
 Base class for types that reference an Objective-C object.
 */
class ObjCObjectRef {
public:
    virtual ~ObjCObjectRef() = default;
    virtual id getValue() const = 0;
    virtual bool isStrong() const = 0;
};

/**
 Refernce of an Objective-C instance that goes through a reference table.
 Reference can be taken weakly or strongly.
 */
class ObjCObjectIndirectRef : public ObjCObjectRef {
public:
    ObjCObjectIndirectRef();
    ObjCObjectIndirectRef(__unsafe_unretained id value);
    ObjCObjectIndirectRef(__unsafe_unretained id value, BOOL strongRef);
    ObjCObjectIndirectRef(const ObjCObjectIndirectRef& other);
    ObjCObjectIndirectRef(ObjCObjectIndirectRef&& other);

    ~ObjCObjectIndirectRef() override;

    id getValue() const final;

    bool isStrong() const final;

    ObjCObjectIndirectRef& operator=(const ObjCObjectIndirectRef& other);
    ObjCObjectIndirectRef& operator=(ObjCObjectIndirectRef&& other);

private:
    Valdi::Ref<Valdi::IOS::ObjCReferenceTable> _table;
    Valdi::SequenceID _id;
};

/**
 Reference of an Objective-C instance that is held directly within the reference
 instance itself. The reference is always taken as a strong reference.
 */
class ObjCObjectDirectRef : public ObjCObjectRef {
public:
    ObjCObjectDirectRef();
    ObjCObjectDirectRef(__unsafe_unretained id value);
    ObjCObjectDirectRef(const ObjCObjectDirectRef& other);
    ObjCObjectDirectRef(ObjCObjectDirectRef&& other);

    ~ObjCObjectDirectRef() override;

    id getValue() const final;

    bool isStrong() const final;

    ObjCObjectDirectRef& operator=(const ObjCObjectDirectRef& other);
    ObjCObjectDirectRef& operator=(ObjCObjectDirectRef&& other);

private:
    id _value;
};

class ObjCObject : public ObjCObjectDirectRef, public Valdi::ValueConvertible {
public:
    ObjCObject(id value);
    ~ObjCObject() override;

    Valdi::Value toValue() override;
};

id NSObjectFromValue(const Valdi::Value& value);
Valdi::Value ValueFromNSObject(id object);

Valdi::Value ValueFromFunction(id<SCValdiFunction> function);

Valdi::Value ValueFromNSData(__unsafe_unretained NSData* data);

Valdi::SharedValdiObject ValdiObjectFromNSObject(id object);
id NSObjectFromValdiObject(const Valdi::SharedValdiObject& valdiObject, BOOL wrapIfNeeded);
id NSObjectWrappingValue(const Valdi::Value& value);

id<SCValdiFunction> FunctionFromValue(const Valdi::Value& value);
id<SCValdiFunction> FunctionFromValueFunction(const Valdi::Ref<Valdi::ValueFunction>& valueFunction);

Valdi::Value ValueFromNSData(__unsafe_unretained NSData* data);
NSData* NSDataFromValue(const Valdi::Value& value);

Valdi::Shared<Valdi::ValueConvertible> toValueConvertible(id object);
Valdi::Shared<Valdi::ValueConvertible> toLazyValueConvertible(id object);
Valdi::Shared<Valdi::ValueConvertible> toLazyValueConvertible(const Valdi::Value& value);

Valdi::Ref<Valdi::ValueTypedProxyObject> ProxyObjectFromNSObject(id object,
                                                                 const Valdi::Ref<Valdi::ValueTypedObject>& typedObject,
                                                                 BOOL strongRef);
id NSObjectFromProxyObject(const Valdi::ValueTypedProxyObject& proxyObject);

NSData* NSDataFromBuffer(const Valdi::BytesView& buffer);
Valdi::BytesView BufferFromNSData(__unsafe_unretained NSData* data);

NSURL* NSURLFromString(const Valdi::StringBox& urlString);

NSString* NSStringFromStringView(const std::string_view& str);
NSString* NSStringFromUTF16StringView(const std::u16string_view& str);
NSString* NSStringFromStaticString(const Valdi::StaticString& str);

Valdi::Ref<Valdi::StaticString> StaticStringFromNSString(NSString* str);

NSError* NSErrorFromError(const Valdi::Error& error);
Valdi::Error ErrorFromNSError(NSError* error);

void NSExceptionThrowFromError(const Valdi::Error& error) __attribute__((noreturn));
Valdi::Error ErrorFromNSException(NSException* exception);

} // namespace ValdiIOS
