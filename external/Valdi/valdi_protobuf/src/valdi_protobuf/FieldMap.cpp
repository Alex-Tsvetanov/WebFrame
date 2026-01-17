//
//  FieldMap.cpp
//  valdi_protobuf-pc
//
//  Created by Simon Corsin on 6/16/23.
//

#include "valdi_protobuf/FieldMap.hpp"
#include "valdi_core/cpp/Constants.hpp"
#include <algorithm>

namespace Valdi::Protobuf {

FieldMap::FieldMap() {
    new (&_storage)(Vector)();
}

FieldMap::~FieldMap() {
    destruct();
}

void FieldMap::destruct() {
    if (VALDI_UNLIKELY(_isMap)) {
        getMap().~Map();
    } else {
        getVec().~Vector();
    }
    _isMap = false;
}

FieldMap::FieldMap(const FieldMap& other) : _isMap(other._isMap) {
    if (VALDI_UNLIKELY(_isMap)) {
        new (&_storage)(Map)(other.getMap());
    } else {
        new (&_storage)(Vector)(other.getVec());
    }
}

FieldMap::FieldMap(FieldMap&& other) noexcept : _isMap(other._isMap) {
    if (VALDI_UNLIKELY(_isMap)) {
        new (&_storage)(Map)(std::move(other.getMap()));
    } else {
        new (&_storage)(Vector)(std::move(other.getVec()));
    }

    other._isMap = false;
    new (&other._storage)(Vector)(other.getVec());
}

FieldMap& FieldMap::operator=(const FieldMap& other) {
    if (this != &other) {
        destruct();

        _isMap = other._isMap;
        if (VALDI_UNLIKELY(_isMap)) {
            new (&_storage)(Map)(other.getMap());
        } else {
            new (&_storage)(Vector)(other.getVec());
        }
    }

    return *this;
}

FieldMap& FieldMap::operator=(FieldMap&& other) noexcept {
    if (this != &other) {
        destruct();

        _isMap = other._isMap;
        if (VALDI_UNLIKELY(_isMap)) {
            new (&_storage)(Map)(std::move(other.getMap()));
        } else {
            new (&_storage)(Vector)(std::move(other.getVec()));
        }

        other._isMap = false;
        new (&other._storage)(Vector)(other.getVec());
    }

    return *this;
}

Field& FieldMap::operator[](FieldNumber i) {
    if (VALDI_UNLIKELY(_isMap)) {
        return getMap()[i];
    } else {
        if (i >= FieldMap::kMaxVecSize) {
            return toMap()[i];
        }

        auto& vec = getVec();
        while (i >= vec.size()) {
            vec.emplace_back();
        }
        return vec[i];
    }
}

bool FieldMap::erase(FieldNumber i) {
    if (VALDI_UNLIKELY(_isMap)) {
        auto& map = getMap();
        auto it = map.find(i);
        if (it == map.end()) {
            return false;
        }
        map.erase(it);
        return true;
    } else {
        auto& vec = getVec();
        if (i >= vec.size()) {
            return false;
        }
        auto& item = vec[i];
        if (item.isUnset()) {
            return false;
        }
        item = Field();
        return true;
    }
}

Field* FieldMap::find(FieldNumber i) {
    if (VALDI_UNLIKELY(_isMap)) {
        auto& map = getMap();
        auto it = map.find(i);
        if (it == map.end()) {
            return nullptr;
        }
        return &it->second;
    } else {
        auto& vec = getVec();
        if (i >= vec.size()) {
            return nullptr;
        }
        auto& item = vec[i];
        if (item.isUnset()) {
            return nullptr;
        }
        return &item;
    }
}

const Field* FieldMap::find(FieldNumber i) const {
    if (VALDI_UNLIKELY(_isMap)) {
        const auto& map = getMap();
        const auto& it = map.find(i);
        if (it == map.end()) {
            return nullptr;
        }
        return &it->second;
    } else {
        const auto& vec = getVec();
        if (i >= vec.size()) {
            return nullptr;
        }
        const auto& item = vec[i];
        if (item.isUnset()) {
            return nullptr;
        }
        return &item;
    }
}

void FieldMap::clear() {
    if (VALDI_UNLIKELY(_isMap)) {
        getMap().clear();
    } else {
        getVec().clear();
    }
}

std::vector<FieldNumber> FieldMap::sortedFieldNumbers() const {
    auto entries = getEntries();

    std::vector<FieldNumber> output;
    output.reserve(entries.size());

    for (const auto& entry : entries) {
        output.emplace_back(entry.number);
    }

    return output;
}

FieldMap::EntryList FieldMap::getEntries() const {
    FieldMap::EntryList output;

    if (VALDI_UNLIKELY(_isMap)) {
        forEachMap([&](const auto& entry) {
            auto it = std::upper_bound(
                output.begin(), output.end(), entry, [=](const FieldMap::Entry& a, const FieldMap::Entry& b) {
                    return a.number < b.number;
                });

            output.emplace(it, entry);
        });
    } else {
        forEachVec([&](const auto& entry) { output.emplace_back(entry); });
    }

    return output;
}

bool FieldMap::isUsingMap() const {
    return _isMap;
}

FieldMap::Map& FieldMap::toMap() {
    FieldMap::Vector vec(std::move(getVec()));

    new (&_storage)(FieldMap::Map)();
    _isMap = true;
    auto& map = getMap();

    size_t index = 0;
    for (auto& item : vec) {
        if (!item.isUnset()) {
            map[index] = std::move(item);
        }
        index++;
    }

    return map;
}

} // namespace Valdi::Protobuf
