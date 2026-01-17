//
//  StringBox.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/InternedStringImpl.hpp"
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#define STRINGIFY(a) __STRINGIFY(a)
#define __STRINGIFY(a) #a

namespace Valdi {

class StringBox {
public:
    explicit StringBox(Ref<InternedStringImpl> str) noexcept;
    StringBox() noexcept;
    StringBox(const StringBox& other) noexcept;
    StringBox(StringBox&& other) noexcept;

    const char* getCStr() const noexcept;

    std::string_view toStringView() const noexcept;
    /**
     Convert the StringBox to an std string, potentially doing a heap allocation in the process.
     Use toStringView() instead when possible
     */
    std::string slowToString() const noexcept;

    StringBox& operator=(const StringBox& other) noexcept;
    StringBox& operator=(StringBox&& other) noexcept;
    bool operator==(const Valdi::StringBox& other) const noexcept;
    bool operator!=(const Valdi::StringBox& other) const noexcept;
    bool operator==(const std::string_view& strView) const noexcept;
    bool operator!=(const std::string_view& strView) const noexcept;

    char operator[](size_t i) const;

    // IMPORTANT NOTE: This does not compare using characters comparison. It only uses the backing
    // string pointer. As such this is only useful to have a consistent ordering of StringBox
    // objects for fast querying.
    bool operator<(const Valdi::StringBox& other) const noexcept;

    size_t hash() const noexcept;

    size_t length() const noexcept;

    bool isEmpty() const noexcept;

    bool isNull() const noexcept;

    const Ref<InternedStringImpl>& getInternedString() const noexcept;

    friend std::ostream& operator<<(std::ostream& os, const StringBox& d) noexcept;

    static size_t makeHash(const char* str, size_t len) noexcept;
    static size_t makeHash(const char16_t* str, size_t len) noexcept;
    static size_t makeHash(const std::string_view& str) noexcept;

    bool hasSuffix(const char* suffix) const noexcept;
    bool hasSuffix(const StringBox& suffix) const noexcept;
    bool hasPrefix(const char* prefix) const noexcept;
    bool hasPrefix(const StringBox& prefix) const noexcept;

    bool contains(const std::string_view& str) const noexcept;
    bool contains(char character) const noexcept;
    std::optional<size_t> indexOf(char character) const noexcept;
    std::optional<size_t> lastIndexOf(char character) const noexcept;
    std::optional<size_t> find(const std::string_view& str) const noexcept;

    StringBox replacing(char c, char replacement) const noexcept;
    StringBox lowercased() const noexcept;

    StringBox append(const StringBox& other) const noexcept;
    StringBox append(const std::string_view& other) const noexcept;
    StringBox prepend(const StringBox& other) const noexcept;
    StringBox prepend(const std::string_view& other) const noexcept;
    StringBox& operator+=(const StringBox& other) noexcept;
    StringBox& operator+=(const std::string_view& other) noexcept;
    StringBox operator+(const StringBox& other) const noexcept;
    StringBox operator+(const std::string_view& other) const noexcept;
    StringBox substring(size_t fromIndex) const noexcept;
    StringBox substring(size_t fromIndex, size_t toIndex) const noexcept;

    std::optional<uint32_t> toUInt32() const noexcept;

    std::pair<StringBox, StringBox> split(size_t index) const noexcept;

    std::vector<StringBox> split(char separator) const noexcept;

    std::vector<StringBox> split(char separator, bool omitEmptySubsequences) const noexcept;

    static StringBox join(const std::vector<StringBox>& strings, std::string_view separator);
    static StringBox join(std::initializer_list<StringBox> strings, std::string_view separator) {
        return join(strings.begin(), strings.size(), separator);
    }

    static StringBox join(const StringBox* strings, size_t length, std::string_view separator);

    static StringBox fromCString(const char* str);
    static StringBox fromString(const std::string& str);
    static StringBox fromString(std::string_view str);

    /**
     Returns a trimmed String by removing whitespaces on both ends of the string.
     */
    StringBox trimmed() const noexcept;

    static const StringBox& emptyString();

private:
    Ref<InternedStringImpl> _internedString;
};

bool operator==(const std::string& stdString, const Valdi::StringBox& other) noexcept;

std::string indentString(const std::string& input) noexcept;
void stringReplace(std::string& input, std::string_view searchPattern, std::string_view replacement) noexcept;

} // namespace Valdi

namespace std {

template<>
struct hash<Valdi::StringBox> {
    std::size_t operator()(const Valdi::StringBox& k) const noexcept;
};

} // namespace std
