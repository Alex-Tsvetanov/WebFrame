//
//  ValueSchemaTypeReference.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/17/23.
//

#include "valdi_core/cpp/Schema/ValueSchemaTypeReference.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include <boost/functional/hash.hpp>
#include <fmt/format.h>

namespace Valdi {

constexpr auto kUnpositionalSentinel = std::numeric_limits<ValueSchemaTypeReference::Position>::max();

ValueSchemaTypeReference::ValueSchemaTypeReference() : _position(kUnpositionalSentinel) {}
ValueSchemaTypeReference::ValueSchemaTypeReference(ValueSchemaTypeReferenceTypeHint typeHint, const StringBox& name)
    : _name(name), _typeHint(typeHint), _position(kUnpositionalSentinel) {}
ValueSchemaTypeReference::ValueSchemaTypeReference(ValueSchemaTypeReference::Position position) : _position(position) {}

ValueSchemaTypeReference::~ValueSchemaTypeReference() = default;

bool ValueSchemaTypeReference::isPositional() const {
    return _position != kUnpositionalSentinel;
}

bool ValueSchemaTypeReference::isNamed() const {
    return !_name.isNull();
}

StringBox ValueSchemaTypeReference::getName() const {
    return _name;
}

ValueSchemaTypeReference::Position ValueSchemaTypeReference::getPosition() const {
    return _position;
}

ValueSchemaTypeReferenceTypeHint ValueSchemaTypeReference::getTypeHint() const {
    return _typeHint;
}

std::string ValueSchemaTypeReference::toString() const {
    if (isPositional()) {
        return std::to_string(_position);
    } else {
        return fmt::format("'{}'", _name.toStringView());
    }
}

size_t ValueSchemaTypeReference::hash() const {
    if (isPositional()) {
        return _position;
    } else {
        auto h = _name.hash();
        boost::hash_combine(h, _typeHint);

        return h;
    }
}

ValueSchemaTypeReference ValueSchemaTypeReference::positional(Position position) {
    return ValueSchemaTypeReference(position);
}

ValueSchemaTypeReference ValueSchemaTypeReference::named(const StringBox& name) {
    return named(ValueSchemaTypeReferenceTypeHintUnknown, name);
}

ValueSchemaTypeReference ValueSchemaTypeReference::named(ValueSchemaTypeReferenceTypeHint typeHint,
                                                         const StringBox& name) {
    return ValueSchemaTypeReference(typeHint, name);
}

bool ValueSchemaTypeReference::operator==(const ValueSchemaTypeReference& other) const {
    return _name == other._name && _typeHint == other._typeHint && _position == other._position;
}

bool ValueSchemaTypeReference::operator!=(const ValueSchemaTypeReference& other) const {
    return !(*this == other);
}

} // namespace Valdi

namespace std {

std::size_t hash<Valdi::ValueSchemaTypeReference>::operator()(const Valdi::ValueSchemaTypeReference& k) const noexcept {
    return k.hash();
}

} // namespace std
