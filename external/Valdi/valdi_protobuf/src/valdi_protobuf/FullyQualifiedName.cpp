// Copyright © 2024 Snap, Inc. All rights reserved.

#include "valdi_protobuf/FullyQualifiedName.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include <boost/functional/hash.hpp>

namespace Valdi::Protobuf {

FullyQualifiedName::FullyQualifiedName() = default;
FullyQualifiedName::FullyQualifiedName(std::string_view str) {
    auto dotSeparator = str.find_last_of('.');
    if (dotSeparator != std::string_view::npos) {
        auto advanceIndex = dotSeparator + 1;
        _leading = StringCache::getGlobal().makeString(str.data(), dotSeparator);
        _trailing = StringCache::getGlobal().makeString(str.data() + advanceIndex, str.size() - advanceIndex);
    } else {
        _trailing = StringCache::getGlobal().makeString(str);
    }
}

FullyQualifiedName::FullyQualifiedName(StringBox leading, StringBox trailing)
    : _leading(std::move(leading)), _trailing(std::move(trailing)) {}

FullyQualifiedName::~FullyQualifiedName() = default;

void FullyQualifiedName::append(std::string_view components) {
    while (!components.empty()) {
        auto dotIndex = components.find_first_of('.');
        if (dotIndex == std::string_view::npos) {
            appendSingle(components);
            components = std::string_view();
        } else {
            appendSingle(components.substr(0, dotIndex));
            components = components.substr(dotIndex + 1);
        }
    }
}

void FullyQualifiedName::appendSingle(std::string_view component) {
    auto componentInterned = StringCache::getGlobal().makeString(component);
    ;
    if (_leading.isEmpty()) {
        _leading = std::move(_trailing);
    } else {
        _leading = StringBox::join({_leading, _trailing}, ".");
    }

    _trailing = std::move(componentInterned);
}

bool FullyQualifiedName::empty() const {
    return _trailing.isEmpty();
}

std::string FullyQualifiedName::toString() const {
    std::string output;
    toString(output);
    return output;
}

void FullyQualifiedName::toString(std::string& output) const {
    if (!_leading.isEmpty()) {
        output += _leading.toStringView();
        output.append(1, '.');
    }
    output += _trailing.toStringView();
}

bool FullyQualifiedName::operator==(const FullyQualifiedName& other) const {
    return _leading == other._leading && _trailing == other._trailing;
}

bool FullyQualifiedName::operator!=(const FullyQualifiedName& other) const {
    return !(*this == other);
}

bool FullyQualifiedName::canRemoveLastComponent() const {
    return !_leading.isEmpty();
}

FullyQualifiedName FullyQualifiedName::removingLastComponent() const {
    auto leadingStr = _leading.toStringView();
    auto dotSeparator = leadingStr.find_last_of('.');
    if (dotSeparator != std::string_view::npos) {
        auto advanceIndex = dotSeparator + 1;
        return FullyQualifiedName(
            StringCache::getGlobal().makeString(leadingStr.data(), dotSeparator),
            StringCache::getGlobal().makeString(leadingStr.data() + advanceIndex, leadingStr.size() - advanceIndex));
    } else {
        return FullyQualifiedName(StringBox(), _leading);
    }
}

std::string_view FullyQualifiedName::getLeadingComponents() const {
    return _leading.toStringView();
}

std::string_view FullyQualifiedName::getLastComponent() const {
    return _trailing.toStringView();
}

size_t FullyQualifiedName::hash() const {
    std::size_t hash = 0;
    boost::hash_combine(hash, _leading.hash());
    boost::hash_combine(hash, _trailing.hash());

    return hash;
}

std::ostream& operator<<(std::ostream& os, const FullyQualifiedName& d) noexcept {
    return os << d.toString();
}

} // namespace Valdi::Protobuf

namespace std {

std::size_t hash<Valdi::Protobuf::FullyQualifiedName>::operator()(
    const Valdi::Protobuf::FullyQualifiedName& d) const noexcept {
    return d.hash();
}

} // namespace std
