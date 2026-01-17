//
//  FieldMap.hpp
//  valdi_protobuf-pc
//
//  Created by Simon Corsin on 6/16/23.
//

#pragma once

#include "valdi_core/cpp/Constants.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

#include "valdi_protobuf/Field.hpp"
#include "valdi_protobuf/FieldNumber.hpp"

#include <utility>
#include <vector>

namespace Valdi::Protobuf {

class FieldMap {
public:
    /**
     The field map uses a vector for indexes up to 64, and then switches
     to a map when encountering an index higher than this value.
     Most Message types that Snap use have field numbers that are in
     increasing order with 1 increment starting from 1. Using a vector
     for these types of messages improves insert/fetch performance
     compared to using a map, and it can improve memory usage
     as well especially if the message doesn't have a lot of holes
     in its keys.
     */
    static constexpr size_t kMaxVecSize = 64;

    struct Entry {
        FieldNumber number;
        const Field* value;

        constexpr Entry(FieldNumber number, const Field* value) : number(number), value(value) {}
    };

    using EntryList = Valdi::SmallVector<Entry, 32>;

    using Vector = std::vector<Field>;
    using Map = FlatMap<FieldNumber, Field>;

    FieldMap();
    ~FieldMap();

    FieldMap(const FieldMap& other);
    FieldMap(FieldMap&& other) noexcept;

    FieldMap& operator=(const FieldMap& other);
    FieldMap& operator=(FieldMap&& other) noexcept;

    Field& operator[](FieldNumber i);

    bool erase(FieldNumber i);

    Field* find(FieldNumber i);
    const Field* find(FieldNumber i) const;

    void clear();

    template<typename F>
    void forEach(F&& handler) const {
        if (VALDI_UNLIKELY(_isMap)) {
            forEachMap(std::move(handler));
        } else {
            forEachVec(std::move(handler));
        }
    }

    template<typename F>
    void forEachSorted(F&& handler) const {
        if (VALDI_UNLIKELY(_isMap)) {
            auto entries = getEntries();
            for (const auto& entry : entries) {
                handler(entry);
            }
        } else {
            forEachVec(std::move(handler));
        }
    }

    std::vector<FieldNumber> sortedFieldNumbers() const;

    EntryList getEntries() const;

    bool isUsingMap() const;

private:
    using Storage = std::aligned_union<0, FieldMap::Vector, FieldMap::Map>::type;
    Storage _storage;

    bool _isMap = false;

    FieldMap::Map& toMap();

    template<typename F>
    void forEachMap(F&& handler) const {
        for (const auto& it : getMap()) {
            if (!it.second.isUnset()) {
                handler(Entry(it.first, &it.second));
            }
        }
    }

    template<typename F>
    void forEachVec(F&& handler) const {
        FieldNumber index = 0;
        for (const auto& item : getVec()) {
            if (!item.isUnset()) {
                handler(Entry(index, &item));
            }
            index++;
        }
    }

    inline FieldMap::Vector& getVec() {
        return *reinterpret_cast<FieldMap::Vector*>(&_storage);
    }

    inline const FieldMap::Vector& getVec() const {
        return *reinterpret_cast<const FieldMap::Vector*>(&_storage);
    }

    inline FieldMap::Map& getMap() {
        return *reinterpret_cast<FieldMap::Map*>(&_storage);
    }

    inline const FieldMap::Map& getMap() const {
        return *reinterpret_cast<const FieldMap::Map*>(&_storage);
    }

    inline void destruct();
};

} // namespace Valdi::Protobuf
