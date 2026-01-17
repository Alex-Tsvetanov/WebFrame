//
//  ValueSchemaRegistryKey.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/18/23.
//

#include "valdi_core/cpp/Schema/ValueSchemaRegistryKey.hpp"

namespace Valdi {

ValueSchemaRegistryKey::ValueSchemaRegistryKey(const ValueSchema& schemaKey)
    : _schemaKey(schemaKey), _hash(schemaKey.hash()) {}
ValueSchemaRegistryKey::~ValueSchemaRegistryKey() = default;

bool ValueSchemaRegistryKey::operator==(const ValueSchemaRegistryKey& other) const {
    return _schemaKey == other._schemaKey;
}

bool ValueSchemaRegistryKey::operator!=(const ValueSchemaRegistryKey& other) const {
    return !(*this == other);
}

const ValueSchema& ValueSchemaRegistryKey::getSchemaKey() const {
    return _schemaKey;
}

size_t ValueSchemaRegistryKey::hash() const {
    return _hash;
}

} // namespace Valdi
