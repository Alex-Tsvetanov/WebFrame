//
//  AssetKey.cpp
//  valdi
//
//  Created by Simon Corsin on 6/28/21.
//

#include "valdi/runtime/Resources/AssetKey.hpp"
#include "valdi/runtime/Resources/Bundle.hpp"

#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include <boost/functional/hash.hpp>

namespace Valdi {

AssetKey::AssetKey(const Ref<Bundle>& bundle, const StringBox& path) : _bundle(bundle), _pathOrUrl(path) {}
AssetKey::AssetKey(const StringBox& url) : _pathOrUrl(url) {}
AssetKey::~AssetKey() = default;

const Ref<Bundle>& AssetKey::getBundle() const {
    return _bundle;
}

const StringBox& AssetKey::getPath() const {
    if (isURL()) {
        return StringBox::emptyString();
    }
    return _pathOrUrl;
}

const StringBox& AssetKey::getUrl() const {
    if (!isURL()) {
        return StringBox::emptyString();
    }

    return _pathOrUrl;
}

bool AssetKey::isURL() const {
    return _bundle == nullptr;
}

bool AssetKey::operator==(const AssetKey& other) const {
    return (_bundle == other._bundle) && _pathOrUrl == other._pathOrUrl;
}

bool AssetKey::operator!=(const AssetKey& other) const {
    return !(*this == other);
}

StringBox AssetKey::toString() const {
    if (isURL()) {
        return getUrl();
    } else {
        return STRING_FORMAT("{}/{}", getBundle()->getName(), getPath());
    }
}

std::ostream& operator<<(std::ostream& os, const Valdi::AssetKey& k) noexcept {
    os << k.toString();
    return os;
}

} // namespace Valdi

namespace std {

std::size_t hash<Valdi::AssetKey>::operator()(const Valdi::AssetKey& k) const noexcept {
    if (k.isURL()) {
        return k.getUrl().hash();
    } else {
        std::size_t hash = 0;
        boost::hash_combine(hash, k.getBundle()->getName().hash());
        boost::hash_combine(hash, k.getPath().hash());
        return hash;
    }
}

} // namespace std
