//
//  StringCache.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/7/18.
//

#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Text/UTF16Utils.hpp"
#include <codecvt>
#include <iostream>
#include <vector>

namespace Valdi {

static size_t makePHMapHash(size_t hash) {
    // Same function used by phmap internally, so that the find variant
    // that takes a hash parameter matches what the hasher function does.
    return phmap::phmap_mix<sizeof(size_t)>()(hash);
}

std::size_t StringCacheHash::operator()(const StringCacheEntry& entry) const {
    return entry.impl->getHash();
}

bool StringCacheEqual::operator()(const Valdi::StringCacheEntry& left, const Valdi::StringCacheEntry& right) const {
    return left.impl == right.impl;
}

bool StringCacheEqual::operator()(const Valdi::StringCacheEntry& left, const std::string_view& right) const {
    return left.impl->toStringView() == right;
}

bool StringCacheEqual::operator()(const Valdi::StringCacheEntry& left, const Valdi::InternedStringImpl* right) const {
    return left.impl == right;
}

StringCache::StringCache() = default;

StringBox StringCache::makeStringFromLiteral(const std::string_view& str) noexcept {
    return makeString(str);
}

StringBox StringCache::makeString(const char* str, size_t len) noexcept {
    std::string_view key(str, len);

    return makeString(key);
}

StringBox StringCache::makeString(const std::string& str) noexcept {
    return makeString(std::string_view(str));
}

StringBox StringCache::makeString(std::string_view strView) noexcept {
    if (strView.empty()) {
        return StringBox();
    }

    auto hash = StringBox::makeHash(strView);

    std::lock_guard<Mutex> guard(_mutex);

    const auto& it = findEntry(strView, hash);
    if (it != _table.end()) {
        auto locked = it->impl->lock();
        if (locked != nullptr) {
            return StringBox(Ref<InternedStringImpl>(std::move(locked)));
        } else {
            _table.erase(it);
        }
    }

    return insertString(strView, hash);
}

StringBox StringCache::makeStringFromUTF16(const char16_t* utf16String, size_t len) noexcept {
    auto pair = utf16ToUtf8(utf16String, len);
    return makeString(pair.first, pair.second);
}

StringCache& StringCache::getGlobal() noexcept {
    static auto* kCache = new StringCache();
    return *kCache;
}

StringBox StringCache::makeStringFromCLiteral(const char* cStr) noexcept {
    return getGlobal().makeStringFromLiteral(cStr);
}

StringBox StringCache::insertString(std::string_view str, size_t hash) {
    auto internedString = InternedStringImpl::make(str.data(), str.size(), hash);

    _table.emplace(internedString.get());

    return StringBox(Ref<InternedStringImpl>(std::move(internedString)));
}

void StringCache::removeString(const InternedStringImpl* internedString) {
    std::lock_guard<Mutex> guard(_mutex);

    const auto& it = _table.find(internedString, makePHMapHash(internedString->getHash()));
    if (it != _table.end()) {
        _table.erase(it);
    }
}

std::unique_lock<Mutex> StringCache::lock() {
    return std::unique_lock<Mutex>(_mutex);
}

std::vector<StringBox> StringCache::all() const {
    std::vector<StringBox> out;

    std::lock_guard<Mutex> guard(_mutex);
    out.reserve(_table.size());

    for (const auto& it : _table) {
        auto locked = it.impl->lock();
        if (locked != nullptr) {
            out.emplace_back(Ref<InternedStringImpl>(std::move(locked)));
        }
    }

    return out;
}

StringCache::StringTable::const_iterator StringCache::findEntry(const std::string_view& str, size_t hash) const {
    return _table.find(str, makePHMapHash(hash));
}

} // namespace Valdi
