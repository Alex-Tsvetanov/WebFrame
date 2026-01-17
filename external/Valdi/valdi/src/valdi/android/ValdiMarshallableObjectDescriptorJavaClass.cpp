//
//  ValdiMarshallableObjectDescriptorJavaClass.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/22/23.
//

#include "valdi/android/ValdiMarshallableObjectDescriptorJavaClass.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"

namespace ValdiAndroid {

ValdiMarshallableObjectDescriptorJavaClass::ValdiMarshallableObjectDescriptorJavaClass(JavaClass&& cls)
    : cls(std::move(cls)) {}

ValdiMarshallableObjectDescriptorJavaClass ValdiMarshallableObjectDescriptorJavaClass::make() {
    auto className = STRING_LITERAL("com/snap/valdi/schema/ValdiMarshallableObjectDescriptor");
    auto cls = JavaClass::resolveOrAbort(JavaEnv(), className.getCStr());

    ValdiMarshallableObjectDescriptorJavaClass out(std::move(cls));

    auto expectedReturnType = Valdi::ValueSchema::typeReference(Valdi::ValueSchemaTypeReference::named(className));

    auto classType =
        Valdi::ValueSchema::typeReference(Valdi::ValueSchemaTypeReference::named(STRING_LITERAL("java/lang/Class")));

    out.getDescriptorForClassMethod =
        out.cls.getStaticMethod("getDescriptorForClass", expectedReturnType, &classType, 1, false);

    std::initializer_list<Valdi::ValueSchema> objectImplementsMethodParameters = {
        Valdi::ValueSchema::typeReference(Valdi::ValueSchemaTypeReference::named(STRING_LITERAL("java/lang/Object"))),
        Valdi::ValueSchema::typeReference(
            Valdi::ValueSchemaTypeReference::named(STRING_LITERAL("java/lang/reflect/Method")))};

    out.objectImplementsMethodMethod = out.cls.getStaticMethod("objectImplementsMethod",
                                                               Valdi::ValueSchema::boolean(),
                                                               objectImplementsMethodParameters.begin(),
                                                               objectImplementsMethodParameters.size(),
                                                               false);

    out.typeField = out.cls.getField("type", Valdi::ValueSchema::integer(), false);
    out.schemaField = out.cls.getField("schema", Valdi::ValueSchema::string(), false);
    out.proxyClassField = out.cls.getField("proxyClass", classType, false);
    out.typeReferencesField =
        out.cls.getField("typeReferences", Valdi::ValueSchema::array(Valdi::ValueSchema::string()), false);
    out.propertyReplacementsField = out.cls.getField("propertyReplacements", Valdi::ValueSchema::string(), false);

    return out;
}

const ValdiMarshallableObjectDescriptorJavaClass& ValdiMarshallableObjectDescriptorJavaClass::get() {
    static auto kInstance = make();
    return kInstance;
};

} // namespace ValdiAndroid
