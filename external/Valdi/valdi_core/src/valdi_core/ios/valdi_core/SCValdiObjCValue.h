//
//  SCValdiObjCValue.h
//  valdi_core-ios
//
//  Created by Simon Corsin on 1/31/23.
//

#import "valdi_core/SCValdiFieldValue.h"
#import "valdi_core/cpp/Utils/SmallVector.hpp"
#import <Foundation/Foundation.h>

namespace ValdiIOS {

/**
 A C++ wrapper around a Objective-C value that takes care of retaining/releasing
 the object if the value represents an object, and ensures that the value is
 used with the right type. The ObjCValue holds the underlying type of the value,
 and will abort if the value is used incorrectly, to prevent undefined behaviors.
 */
class ObjCValue {
public:
    ObjCValue();
    ObjCValue(const SCValdiFieldValue& value, SCValdiFieldValueType type);

    ObjCValue(const ObjCValue& other);

    ObjCValue(ObjCValue&& other);

    ~ObjCValue();

    ObjCValue& operator=(ObjCValue&& other) noexcept;

    ObjCValue& operator=(const ObjCValue& other) noexcept;

    int32_t getInt() const;

    int64_t getLong() const;

    bool getBool() const;

    double getDouble() const;

    id getObject() const;

    const void* getPtr() const;

    bool isPtr() const;

    bool isVoid() const;

    SCValdiFieldValue getAsParameterValue(SCValdiFieldValueType expectedType) const;
    SCValdiFieldValue getAsReturnValue(SCValdiFieldValueType expectedType) const;

    static ObjCValue makeInt(int32_t i);

    static ObjCValue makeLong(int64_t l);

    static ObjCValue makeBool(bool b);

    static ObjCValue makeDouble(double d);

    static ObjCValue makeObject(__unsafe_unretained id o);

    static ObjCValue makeUnretainedObject(__unsafe_unretained id o);

    static ObjCValue makePtr(const void* ptr);

    static ObjCValue makeNull();

    /**
     Convert the given ObjCValue array into a SCValdiFieldValue, which is suitable
     for use in a Objective-C block trampoline implementation or as a fields for
     SCValdiMarshallableObject subclasses. The input field descriptors is given
     to ensure that the given values are of the correct type.
     */
    template<typename F>
    static auto toFieldValues(size_t length,
                              const ObjCValue* values,
                              const SCValdiFieldValueDescriptor* fieldDescriptors,
                              F&& handler) -> auto {
        SCValdiFieldValue fieldValues[length];

        for (size_t i = 0; i < length; i++) {
            fieldValues[i] = values[i].getAsParameterValue(fieldDescriptors[i].type);
        }

        return handler(length, fieldDescriptors, &fieldValues[0]);
    }

    /**
     Convert the given ObjCValue array into a SCValdiFieldValue, which is suitable
     for use in a Objective-C block trampoline implementation or as a fields for
     SCValdiMarshallableObject subclasses. The input field types is given
     to ensure that the given values are of the correct type.
     */
    template<typename F>
    static auto toFieldValues(size_t length,
                              const ObjCValue* values,
                              const SCValdiFieldValueType* fieldTypes,
                              F&& handler) -> auto {
        SCValdiFieldValue fieldValues[length];
        for (size_t i = 0; i < length; i++) {
            fieldValues[i] = values[i].getAsParameterValue(fieldTypes[i]);
        }

        return handler(&fieldValues[0]);
    }

    /**
     Convert the given SCValdiFieldValue array into an ObjCValue array.
     */
    template<typename F>
    static auto toObjCValues(size_t length,
                             const SCValdiFieldValue* values,
                             const SCValdiFieldValueType* fieldTypes,
                             F&& handler) -> auto {
        Valdi::SmallVector<ObjCValue, 16> objcValues;
        objcValues.reserve(length);
        for (size_t i = 0; i < length; i++) {
            objcValues.emplace_back(values[i], fieldTypes[i]);
        }

        return handler(objcValues.data());
    }

private:
    SCValdiFieldValue _value;
    SCValdiFieldValueType _type;

    inline void safeRetain();

    inline void safeRelease();
};

} // namespace ValdiIOS
