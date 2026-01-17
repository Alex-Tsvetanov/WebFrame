// Copyright © 2024 Snap, Inc. All rights reserved.

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <ostream>
#include <string>

namespace Valdi::Protobuf {

/**
 * FullyQualifiedName is a class optimized to represent a fully qualified name
 * like "com.snap.valdi.Example". It can be used as a key to a FlatMap.
 */
class FullyQualifiedName {
public:
    FullyQualifiedName();
    FullyQualifiedName(std::string_view str);
    ~FullyQualifiedName();

    void append(std::string_view components);

    bool empty() const;

    bool canRemoveLastComponent() const;

    FullyQualifiedName removingLastComponent() const;

    std::string toString() const;
    void toString(std::string& output) const;

    /**
     Return the components before the last component as a string.
     */
    std::string_view getLeadingComponents() const;

    /**
     Return the last component of the name as a string
     */
    std::string_view getLastComponent() const;

    size_t hash() const;

    friend std::ostream& operator<<(std::ostream& os, const FullyQualifiedName& d) noexcept;

    bool operator==(const FullyQualifiedName& other) const;
    bool operator!=(const FullyQualifiedName& other) const;

private:
    StringBox _leading;
    StringBox _trailing;

    FullyQualifiedName(StringBox leading, StringBox trailing);

    void appendSingle(std::string_view component);
};

} // namespace Valdi::Protobuf

namespace std {

template<>
struct hash<Valdi::Protobuf::FullyQualifiedName> {
    std::size_t operator()(const Valdi::Protobuf::FullyQualifiedName& d) const noexcept;
};

} // namespace std
