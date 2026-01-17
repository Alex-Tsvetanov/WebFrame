//
//  StringBox.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Attributes/AttributeUtils.hpp"
#include "valdi_core/cpp/Utils/AutoMalloc.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <charconv>
#include <fmt/ostream.h>
#include <sstream>

namespace Valdi {

StringBox::StringBox() noexcept = default;

StringBox::StringBox(Ref<InternedStringImpl> str) noexcept : _internedString(std::move(str)) {}

StringBox::StringBox(const StringBox& other) noexcept = default;

StringBox::StringBox(StringBox&& other) noexcept : _internedString(std::move(other._internedString)) {}

const char* StringBox::getCStr() const noexcept {
    if (_internedString == nullptr) {
        return "";
    } else {
        return _internedString->getCStr();
    }
}

std::string_view StringBox::toStringView() const noexcept {
    if (_internedString == nullptr) {
        // NOLINTNEXTLINE(bugprone-string-constructor)
        return std::string_view("", 0);
    } else {
        return _internedString->toStringView();
    }
}

std::string StringBox::slowToString() const noexcept {
    return std::string(toStringView());
}

StringBox& StringBox::operator=(StringBox&& other) noexcept {
    if (this != &other) {
        _internedString = std::move(other._internedString);
    }

    return *this;
}

StringBox& StringBox::operator=(const Valdi::StringBox& other) noexcept = default;

bool StringBox::operator==(const Valdi::StringBox& other) const noexcept {
    // we just need to compare the pointers.
    return _internedString.get() == other._internedString.get();
}

bool StringBox::operator!=(const Valdi::StringBox& other) const noexcept {
    return !(*this == other);
}

bool StringBox::operator==(const std::string_view& strView) const noexcept {
    return toStringView() == strView;
}

bool StringBox::operator!=(const std::string_view& strView) const noexcept {
    return !(*this == strView);
}

bool StringBox::operator<(const Valdi::StringBox& other) const noexcept {
    return _internedString.get() < other._internedString.get();
}

char StringBox::operator[](size_t i) const {
    return getCStr()[i];
}

bool StringBox::contains(char character) const noexcept {
    return indexOf(character).has_value();
}

std::optional<size_t> StringBox::indexOf(char character) const noexcept {
    auto index = toStringView().find(character);
    if (index == std::string_view::npos) {
        return std::nullopt;
    }

    return {index};
}

std::optional<size_t> StringBox::lastIndexOf(char character) const noexcept {
    auto index = toStringView().rfind(character);
    if (index == std::string_view::npos) {
        return std::nullopt;
    }

    return {index};
}

std::optional<size_t> StringBox::find(const std::string_view& str) const noexcept {
    auto index = toStringView().find(str);
    if (index == std::string_view::npos) {
        return std::nullopt;
    }

    return {index};
}

bool StringBox::contains(const std::string_view& str) const noexcept {
    return toStringView().find(str) != std::string_view::npos;
}

size_t StringBox::hash() const noexcept {
    // Use the interned string address as the hash, since
    // it's guaranteed to be unique.
    return reinterpret_cast<size_t>(_internedString.get());
}

std::string indentString(const std::string& input) noexcept {
    std::stringstream ss(input);
    std::string to;
    std::string out;
    bool isFirst = true;

    while (std::getline(ss, to, '\n')) {
        if (!isFirst) {
            out += "\n";
        }
        isFirst = false;
        out += "  ";
        out += to;
    }

    return out;
}

void stringReplace(std::string& input, std::string_view searchPattern, std::string_view replacement) noexcept {
    for (;;) {
        auto index = input.find(searchPattern);
        if (index == std::string::npos) {
            break;
        }

        input.replace(index, searchPattern.size(), replacement);
    }
}

const Ref<InternedStringImpl>& StringBox::getInternedString() const noexcept {
    return _internedString;
}

bool StringBox::isNull() const noexcept {
    return _internedString == nullptr;
}

bool StringBox::isEmpty() const noexcept {
    return _internedString == nullptr || _internedString->length() == 0;
}

size_t StringBox::length() const noexcept {
    return _internedString != nullptr ? _internedString->length() : 0;
}

std::ostream& operator<<(std::ostream& os, const StringBox& d) noexcept {
    return os << d.toStringView();
}

size_t StringBox::makeHash(const char* str, size_t len) noexcept {
    std::string_view strView(str, len);
    return makeHash(strView);
}

size_t StringBox::makeHash(const std::string_view& str) noexcept {
    if (str.empty()) {
        // Keeping 0 as hash for empty string to make them equals to null strings
        return 0;
    }

    return std::hash<std::string_view>()(str);
}

size_t StringBox::makeHash(const char16_t* str, size_t len) noexcept {
    std::u16string_view strView(str, len);
    return std::hash<std::u16string_view>()(strView);
}

bool StringBox::hasPrefix(const char* prefix) const noexcept {
    return Valdi::hasPrefix(toStringView(), 0, prefix);
}

bool StringBox::hasPrefix(const StringBox& prefix) const noexcept {
    return Valdi::hasPrefix(toStringView(), 0, prefix.getCStr());
}

bool StringBox::hasSuffix(const char* suffix) const noexcept {
    return Valdi::hasSuffix(toStringView(), suffix);
}

bool StringBox::hasSuffix(const StringBox& suffix) const noexcept {
    return Valdi::hasSuffix(toStringView(), suffix.getCStr());
}

StringBox StringBox::replacing(char c, char replacement) const noexcept {
    if (!contains(c)) {
        // Avoid copies if character is not found
        return *this;
    }

    auto out = slowToString();

    for (char& existingChar : out) {
        if (existingChar == c) {
            existingChar = replacement;
        }
    }

    return StringCache::getGlobal().makeString(std::move(out));
}

StringBox StringBox::prepend(const StringBox& other) const noexcept {
    return prepend(other.toStringView());
}

StringBox StringBox::prepend(const std::string_view& other) const noexcept {
    if (other.empty()) {
        return *this;
    }

    std::string out;
    out.reserve(other.size() + length());

    out.append(other);
    out.append(toStringView());

    return StringCache::getGlobal().makeString(std::move(out));
}

StringBox StringBox::append(const StringBox& other) const noexcept {
    if (isEmpty()) {
        return other;
    }

    return append(other.toStringView());
}

std::optional<uint32_t> StringBox::toUInt32() const noexcept {
    auto sv = toStringView();
    uint32_t value = 0;

    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (ec == std::errc() && ptr == sv.data() + sv.size()) {
        return value;
    }
    return std::nullopt;
}

StringBox StringBox::append(const std::string_view& other) const noexcept {
    std::string out;
    out.reserve(length() + other.size());

    out.append(toStringView());
    out.append(other);

    return StringCache::getGlobal().makeString(std::move(out));
}

StringBox StringBox::substring(size_t fromIndex) const noexcept {
    return substring(fromIndex, length());
}

StringBox StringBox::substring(size_t fromIndex, size_t toIndex) const noexcept {
    auto strView = std::string_view(getCStr() + fromIndex, toIndex - fromIndex);
    return StringCache::getGlobal().makeString(strView);
}

StringBox StringBox::fromCString(const char* str) {
    return fromString(std::string_view(str));
}

StringBox StringBox::fromString(const std::string& str) {
    return fromString(std::string_view(str));
}

StringBox StringBox::fromString(std::string_view str) {
    return StringCache::getGlobal().makeString(str);
}

StringBox StringBox::join(const std::vector<StringBox>& strings, std::string_view separator) {
    return join(strings.data(), strings.size(), separator);
}

StringBox StringBox::join(const StringBox* strings, size_t length, std::string_view separator) {
    if (length == 1) {
        return strings[0];
    }

    size_t totalLength = 0;

    for (size_t i = 0; i < length; i++) {
        if (i != 0) {
            totalLength += separator.size();
        }
        totalLength += strings[i].length();
    }

    AutoMalloc<char, 64> out(totalLength);

    auto* current = out.data();

    for (size_t i = 0; i < length; i++) {
        if (i != 0) {
            for (auto c : separator) {
                *current = c;
                current++;
            }
        }
        auto length = strings[i].length();
        std::memcpy(current, strings[i].getCStr(), length);
        current += length;
    }

    return StringCache::getGlobal().makeString(out.data(), totalLength);
}

StringBox StringBox::lowercased() const noexcept {
    auto hasUppercase = false;

    auto strView = toStringView();
    for (auto c : strView) {
        if (std::isupper(c) != 0) {
            hasUppercase = true;
            break;
        }
    }

    if (!hasUppercase) {
        return *this;
    }

    std::string out;
    out.reserve(length());

    for (auto c : strView) {
        out.push_back(std::tolower(c));
    }

    return StringCache::getGlobal().makeString(std::move(out));
}

StringBox& StringBox::operator+=(const StringBox& other) noexcept {
    return (*this) = append(other);
}

StringBox& StringBox::operator+=(const std::string_view& other) noexcept {
    return (*this) = append(other);
}

StringBox StringBox::operator+(const StringBox& other) const noexcept {
    return append(other);
}

StringBox StringBox::operator+(const std::string_view& other) const noexcept {
    return append(other);
}

std::pair<StringBox, StringBox> StringBox::split(size_t index) const noexcept {
    SC_ASSERT(index < length());

    auto left = Valdi::StringCache::getGlobal().makeString(getCStr(), index);
    auto right = Valdi::StringCache::getGlobal().makeString(getCStr() + index + 1, length() - index - 1);

    return std::make_pair(std::move(left), std::move(right));
}

std::vector<StringBox> StringBox::split(char separator) const noexcept {
    return split(separator, false);
}

std::vector<StringBox> StringBox::split(char separator, bool omitEmptySubsequences) const noexcept {
    size_t rangeStart = 0;
    size_t i = 0;
    auto length = this->length();

    const char* str = getCStr();
    std::vector<StringBox> out;

    while (i < length) {
        if (*str == separator) {
            // Ignore empty range.
            if (!omitEmptySubsequences || rangeStart != i) {
                auto subStr = Valdi::StringCache::getGlobal().makeString(getCStr() + rangeStart, i - rangeStart);
                out.emplace_back(std::move(subStr));
            }

            rangeStart = i + 1;
        }

        str++;
        i++;
    }

    if (rangeStart != i) {
        if (rangeStart == 0) {
            // We had no multiple, pass the original string
            out.emplace_back(*this);
        } else {
            auto subStr = Valdi::StringCache::getGlobal().makeString(getCStr() + rangeStart, i - rangeStart);
            out.emplace_back(std::move(subStr));
        }
    }

    return out;
}

bool operator==(const std::string& stdString, const Valdi::StringBox& other) noexcept {
    return other.toStringView() == stdString;
}

StringBox StringBox::trimmed() const noexcept {
    size_t start = 0;
    size_t end = length();

    auto strView = toStringView();

    while (start < end && Valdi::hasPrefix(strView, start, " ")) {
        start++;
    }

    while (end > start && Valdi::hasSuffix(strView, " ")) {
        end--;
        strView = std::string_view(getCStr(), end);
    }

    if (start == 0 && end == length()) {
        // Nothing changed.
        return *this;
    }

    return StringCache::getGlobal().makeString(std::string_view(getCStr() + start, end - start));
}

const StringBox& StringBox::emptyString() {
    static auto kEmptyString = StringBox();
    return kEmptyString;
}

} // namespace Valdi

namespace std {

std::size_t hash<Valdi::StringBox>::operator()(const Valdi::StringBox& k) const noexcept {
    return k.hash();
}

} // namespace std
