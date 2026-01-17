//
//  JavaEnumClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/15/23.
//

#pragma once

#include "valdi/android/JavaClassDelegate.hpp"

namespace ValdiAndroid {

class IndirectJavaGlobalRef;

/**
 Concrete implementation of JavaClassDelegate which integrates the PlatformEnumClassDelegate API.
 Allows the ValueMarshallerRegistry to create and resolve enum values from a Java enum class.
 */
class JavaEnumClassDelegate : public Valdi::PlatformEnumClassDelegate<JavaValue>, public JavaClassDelegate {
public:
    ~JavaEnumClassDelegate() override;
    JavaValue newEnum(size_t enumCaseIndex, bool asBoxed, Valdi::ExceptionTracker& exceptionTracker) final;

    Valdi::Value enumCaseToValue(const JavaValue& enumeration,
                                 bool isBoxed,
                                 Valdi::ExceptionTracker& exceptionTracker) final;

    void setEnumCase(size_t index, const JavaValue& javaValue);
    JavaValue getEnumCase(size_t index) const;

    static Valdi::Ref<JavaEnumClassDelegate> make(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                                                  jclass cls,
                                                  const Valdi::Ref<Valdi::EnumSchema>& enumSchema);

private:
    Valdi::Ref<Valdi::EnumSchema> _enumSchema;
    friend Valdi::InlineContainerAllocator<JavaEnumClassDelegate, IndirectJavaGlobalRef>;

    JavaEnumClassDelegate(Valdi::ValueSchemaRegistrySchemaIdentifier identifier,
                          jclass cls,
                          const Valdi::Ref<Valdi::EnumSchema>& enumSchema);
};

} // namespace ValdiAndroid
