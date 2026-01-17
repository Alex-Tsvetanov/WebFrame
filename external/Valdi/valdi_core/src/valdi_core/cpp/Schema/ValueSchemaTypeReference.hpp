//
//  ValueSchemaTypeReference.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/17/23.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

enum ValueSchemaTypeReferenceTypeHint : uint8_t {
    ValueSchemaTypeReferenceTypeHintUnknown = 0,
    ValueSchemaTypeReferenceTypeHintObject,
    ValueSchemaTypeReferenceTypeHintEnum,
    ValueSchemaTypeReferenceTypeHintConverted,
};

constexpr std::string_view kValueSchemaTypeReferenceTypeHintUnknownStr = "unknown";
constexpr std::string_view kValueSchemaTypeReferenceTypeHintObjectStr = "object";
constexpr std::string_view kValueSchemaTypeReferenceTypeHintEnumStr = "enum";
constexpr std::string_view kValueSchemaTypeReferenceTypeHintConvertedStr = "converted";

class ValueSchemaTypeReference {
public:
    using Position = uint8_t;

    // Empty type references are invalid
    ValueSchemaTypeReference();
    ~ValueSchemaTypeReference();

    bool isPositional() const;
    bool isNamed() const;
    StringBox getName() const;
    Position getPosition() const;

    ValueSchemaTypeReferenceTypeHint getTypeHint() const;

    std::string toString() const;

    size_t hash() const;

    static ValueSchemaTypeReference positional(Position position);
    static ValueSchemaTypeReference named(ValueSchemaTypeReferenceTypeHint typeHint, const StringBox& name);
    static ValueSchemaTypeReference named(const StringBox& name);

    bool operator==(const ValueSchemaTypeReference& other) const;
    bool operator!=(const ValueSchemaTypeReference& other) const;

private:
    StringBox _name;
    ValueSchemaTypeReferenceTypeHint _typeHint = ValueSchemaTypeReferenceTypeHintUnknown;
    Position _position;

    ValueSchemaTypeReference(ValueSchemaTypeReferenceTypeHint typeHint, const StringBox& name);
    ValueSchemaTypeReference(Position position);
};

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::ValueSchemaTypeReference> {
    std::size_t operator()(const Valdi::ValueSchemaTypeReference& k) const noexcept;
};

} // namespace std
