//
//  ValueSchemaRegistryKey.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/18/23.
//

#pragma once

#include "valdi_core/cpp/Schema/ValueSchema.hpp"

namespace Valdi {

/**
 ValueSchemaRegistryKey represents the key for a ValueSchema within a ValueSchemaRegistry.
 The key is represented as a ValueSchema, for example a registered type named 'HelloWorld'
 will have the key ref:'HelloWorld'. Generic types instances will have a
 generic type reference as their key, for example HelloWorld<string, double> will have
 the key genref:'HelloWorld'<string, double>
 ValueSchemaRegistryKey just wraps the ValueSchema and caches the hash.
 */
class ValueSchemaRegistryKey {
public:
    explicit ValueSchemaRegistryKey(const ValueSchema& schemaKey);
    ~ValueSchemaRegistryKey();

    const ValueSchema& getSchemaKey() const;

    bool operator==(const ValueSchemaRegistryKey& other) const;
    bool operator!=(const ValueSchemaRegistryKey& other) const;

    size_t hash() const;

private:
    ValueSchema _schemaKey;
    size_t _hash;
};

} // namespace Valdi
