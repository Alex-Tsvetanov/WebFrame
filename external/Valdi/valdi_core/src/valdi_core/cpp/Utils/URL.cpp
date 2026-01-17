//
//  URL.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 7/12/21.
//

#include "valdi_core/cpp/Utils/URL.hpp"

namespace Valdi {

URL::URL(const StringBox& url) : _url(url) {
    auto schemeIndex = url.find("://");
    if (schemeIndex) {
        _scheme = url.substring(0, schemeIndex.value());
        _path = url.substring(schemeIndex.value() + 3);
    } else {
        if (url.hasPrefix("data:")) {
            _scheme = url.substring(0, 4);
        }
        _path = url;
    }
}

URL::URL(StringBox&& scheme, StringBox&& path) : _scheme(std::move(scheme)), _path(std::move(path)) {
    if (!_scheme.isEmpty()) {
        _url = _scheme.append("://").append(_path);
    } else {
        _url = _path;
    }
}

URL::~URL() = default;

const StringBox& URL::getScheme() const {
    return _scheme;
}

const StringBox& URL::getUrl() const {
    return _url;
}

const StringBox& URL::getPath() const {
    return _path;
}

bool URL::operator==(const URL& other) const {
    return _url == other._url;
}

bool URL::operator!=(const URL& other) const {
    return !(*this == other);
}

URL URL::withLastPathComponentRemoved() const {
    // If path is empty or has no slashes, we can't remove a component
    auto lastSlashPos = _path.lastIndexOf('/');
    if (!lastSlashPos) {
        return *this;
    }

    // If path ends with slash, find the previous slash (to avoid empty last component)
    if (lastSlashPos.value() == _path.length() - 1) {
        auto prevSlashPos = _path.substring(0, lastSlashPos.value()).lastIndexOf('/');
        if (prevSlashPos) {
            lastSlashPos = prevSlashPos;
        }
    }

    return URL(_scheme.substring(0), _path.substring(0, lastSlashPos.value()));
}

} // namespace Valdi
