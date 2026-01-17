//
//  ResourceId.cpp
//  ValdiRuntime
//
//  Created by Brandon Francis on 7/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include <boost/functional/hash.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace Valdi {

ResourceId::ResourceId(StringBox resourcePath) : resourcePath(std::move(resourcePath)) {}

ResourceId::ResourceId(StringBox bundleName, StringBox resourcePath)
    : bundleName(std::move(bundleName)), resourcePath(std::move(resourcePath)) {}

bool ResourceId::operator==(const Valdi::ResourceId& other) const {
    return bundleName == other.bundleName && resourcePath == other.resourcePath;
}

bool ResourceId::operator!=(const Valdi::ResourceId& other) const {
    return !(*this == other);
}

bool ResourceId::operator<(const Valdi::ResourceId& other) const {
    if (bundleName != other.bundleName) {
        return bundleName < other.bundleName;
    }

    return resourcePath < other.resourcePath;
}

std::string ResourceId::toAbsolutePath() const {
    if (bundleName.isEmpty()) {
        return resourcePath.slowToString();
    }

    std::string out;
    out.reserve(bundleName.length() + resourcePath.length() + 1);
    out.append(bundleName.toStringView());
    out += '/';
    out.append(resourcePath.toStringView());

    return out;
}

std::ostream& operator<<(std::ostream& os, const ResourceId& d) noexcept {
    return os << d.toAbsolutePath();
}

} // namespace Valdi

namespace std {

std::size_t hash<Valdi::ResourceId>::operator()(const Valdi::ResourceId& k) const {
    std::size_t hash = 0;
    boost::hash_combine(hash, k.bundleName.hash());
    boost::hash_combine(hash, k.resourcePath.hash());

    return hash;
}

} // namespace std
