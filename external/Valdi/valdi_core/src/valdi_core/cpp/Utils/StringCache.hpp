//
//  StringCache.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/7/18.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatSet.hpp"
#include "valdi_core/cpp/Utils/InternedStringImpl.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#define STRING_LITERAL(str) Valdi::StringCache::makeStringFromCLiteral(str)
#define STRING_FORMAT(__format, ...) Valdi::StringCache::getGlobal().makeString(fmt::format((__format), __VA_ARGS__))

#define STRING_CONST(name, str)                                                                                        \
    const Valdi::StringBox& name() {                                                                                   \
        static auto kValue = STRING_LITERAL(str);                                                                      \
        return kValue;                                                                                                 \
    }

namespace Valdi {

static inline std::string_view convertInline(const char* str) {
    return std::string_view(str, strlen(str));
}

static inline std::string_view convertInline(const std::string& str) {
    return std::string_view(str.data(), str.length());
}

static inline std::string_view convertInline(const StringBox& str) {
    return std::string_view(str.getCStr(), str.length());
}

static inline std::string_view convertInline(const std::string_view& str) {
    return str;
}

struct StringCacheEntry {
    InternedStringImpl* impl;

    inline StringCacheEntry(InternedStringImpl* impl) : impl(impl) {}
};

struct StringCacheHash {
    using is_transparent = void;
    std::size_t operator()(const StringCacheEntry& entry) const;
};

struct StringCacheEqual {
    using is_transparent = void;
    bool operator()(const StringCacheEntry& left, const StringCacheEntry& right) const;
    bool operator()(const StringCacheEntry& left, const std::string_view& right) const;
    bool operator()(const StringCacheEntry& left, const InternedStringImpl* right) const;
};

class StringCache {
public:
    StringCache(const StringCache& other) = delete;

    /**
     Returns a string from the given C++ string literal.
     It will use strlen() to figure out the length of the string.
     */
    StringBox makeStringFromLiteral(const std::string_view& str) noexcept;

    /**
     Returns a string from the given UTF8 string and the given length.
     */
    StringBox makeString(const char* str, size_t len) noexcept;

    /**
     Returns a string from the given std string
     */
    StringBox makeString(const std::string& str) noexcept;

    /**
     Returns a string from the given string view.
     */
    StringBox makeString(std::string_view strView) noexcept;

    /**
     Make a string given the given utf16 string.
     */
    StringBox makeStringFromUTF16(const char16_t* utf16String, size_t len) noexcept;

    /**
     Exposed for tests only, DO NOT USE
     */
    std::unique_lock<Mutex> lock();

    /**
     Returns all of the strings inside the StringCache
     */
    std::vector<StringBox> all() const;

    static StringCache& getGlobal() noexcept;

    // Shortcut for getGlobal().makeStringFromLiteral()
    static StringBox makeStringFromCLiteral(const char* cStr) noexcept;

private:
    using StringTable =
        phmap::flat_hash_set<StringCacheEntry, StringCacheHash, StringCacheEqual, phmap::Allocator<StringCacheEntry>>;

    StringTable _table;
    mutable Mutex _mutex;

    StringCache();

    // Should be called with a lock already acquired
    StringBox insertString(std::string_view str, size_t hash);
    // Should be called without a lock
    void removeString(const InternedStringImpl* internedString);

    friend InternedStringImpl;

    StringTable::const_iterator findEntry(const std::string_view& str, size_t hash) const;
};

} // namespace Valdi
