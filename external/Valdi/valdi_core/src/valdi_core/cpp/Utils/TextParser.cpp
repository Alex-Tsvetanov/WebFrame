//
//  TextParser.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/20/23.
//

#include "valdi_core/cpp/Utils/TextParser.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <cstdlib>
#include <fmt/format.h>

namespace Valdi {

constexpr size_t kMaxErrorMessageLength = 16;

TextParser::TextParser(std::string_view str) : _str(str) {}
TextParser::~TextParser() = default;

bool TextParser::hasError() const {
    return _error.has_value();
}

Error TextParser::getError() const {
    SC_ASSERT(hasError());
    return TextParser::makeParseError(_str, _error.value().error.toString(), _error.value().position);
}

size_t TextParser::position() const {
    return _position;
}

size_t TextParser::end() const {
    return _str.size();
}

bool TextParser::isAtEnd() const {
    return _position >= _str.size();
}

bool TextParser::ensureIsAtEnd() {
    if (!isAtEnd()) {
        setErrorAtCurrentPosition("Unexpectedly found trailing characters");
        return false;
    }
    return true;
}

bool TextParser::ensureNotAtEnd() {
    if (isAtEnd()) {
        setErrorAtCurrentPosition("Unexpectedly reached end of string");
        return false;
    }
    return true;
}

bool TextParser::skip() {
    if (!ensureNotAtEnd()) {
        return false;
    }

    advance();
    return true;
}

bool TextParser::skipCount(size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (!skip()) {
            return false;
        }
    }
    return true;
}

void TextParser::advance() {
    _position++;
}

std::string_view TextParser::substr(size_t from, size_t to) const {
    SC_ASSERT(to >= from && from <= _str.size() && to <= _str.size());
    return std::string_view(_str.data() + from, to - from);
}

bool TextParser::tryParse(char c) {
    return tryParsePredicate([&](auto currentCharacter) { return currentCharacter == c; });
}

bool TextParser::peek(char c) const {
    return peekPredicate([&](auto currentCharacter) { return currentCharacter == c; });
}

char TextParser::peek() const {
    if (isAtEnd()) {
        return 0;
    } else {
        return _str[_position];
    }
}

bool TextParser::parse(char c) {
    if (!tryParse(c)) {
        setErrorAtCurrentPosition(fmt::format("Expecting character '{}'", c));
        return false;
    }

    return true;
}

bool TextParser::tryParse(std::string_view term) {
    auto positionBefore = _position;
    for (auto c : term) {
        if (!tryParse(c)) {
            _position = positionBefore;
            return false;
        }
    }

    return true;
}

bool TextParser::parse(std::string_view term) {
    if (!tryParse(term)) {
        setErrorAtCurrentPosition(fmt::format("Expecting term '{}'", term));
        return false;
    }

    return true;
}

static inline bool isWhitespace(char c) {
    return c == ' ' || c == '\n';
}

bool TextParser::tryParseWhitespaces() {
    auto parsed = false;

    while (tryParsePredicate(isWhitespace)) {
        parsed = true;
    }

    return parsed;
}

template<typename T, typename F, typename... Args>
static bool doParseNumber(const std::string_view& str, size_t& position, T& output, F&& fn, Args&&... args) {
    const auto* startPtr = str.data() + position;
    const auto* maxEndPtr = str.data() + str.size();
    char* endPtr = nullptr;
    output = fn(startPtr, &endPtr, std::forward<Args>(args)...);
    if (endPtr != nullptr && endPtr != startPtr && endPtr <= maxEndPtr) {
        position += static_cast<size_t>(endPtr - startPtr);
        return true;
    } else {
        return false;
    }
}

std::optional<double> TextParser::parseDouble() {
    double d;

    if (!parseDouble(d)) {
        return std::nullopt;
    }

    return d;
}

std::optional<int> TextParser::parseInt() {
    int i;

    if (!parseInt(i)) {
        return std::nullopt;
    }

    return i;
}

std::optional<uint32_t> TextParser::parseUInt() {
    uint32_t i;

    if (!parseUInt(i)) {
        return std::nullopt;
    }

    return i;
}

std::optional<long long> TextParser::parseHexLong() {
    long long l;

    if (!parseHexLong(l)) {
        return std::nullopt;
    }

    return l;
}

bool TextParser::parseDouble(double& output) {
    if (!doParseNumber<double>(_str, _position, output, strtod)) {
        setErrorAtCurrentPosition("Expecting double");
        return false;
    }
    return true;
}

bool TextParser::parseInt(int& output) {
    long longOutput;
    if (!doParseNumber<long>(_str, _position, longOutput, strtol, 10)) {
        setErrorAtCurrentPosition("Expecting int");
        return false;
    }
    output = static_cast<int>(longOutput);
    return true;
}

bool TextParser::parseHexLong(long long& output) {
    if (!doParseNumber<long long>(_str, _position, output, strtoll, 16)) {
        setErrorAtCurrentPosition("Expecting hex long");
        return false;
    }
    return true;
}

bool TextParser::parseUInt(uint32_t& output) {
    const auto* begin = _str.data() + _position;
    const auto* end = _str.data() + _str.size();
    const auto* it = begin;

    uint32_t result = 0;

    // This naive parser is quite a bit faster than strtol
    while (it != end) {
        auto c = *it;
        if (!(c >= '0' && c <= '9')) {
            break;
        }

        result = (result * 10) + c - '0';
        it++;
    }

    auto visitedLength = it - begin;

    if (visitedLength == 0) {
        setErrorAtCurrentPosition("Expecting int");
        return false;
    }

    _position += visitedLength;
    output = result;

    return true;
}

inline bool isIdentifierChar(char c) {
    if (isalnum(c) != 0) {
        return true;
    }
    if (c == '-' || c == '_' || c == '!') {
        return true;
    }

    return false;
}

std::optional<std::string_view> TextParser::parseIdentifier() {
    auto currentPosition = position();
    tryParseWhitespaces();

    auto str = readWhile(isIdentifierChar);
    if (str.empty()) {
        _position = currentPosition;
        setErrorAtCurrentPosition("Expecting identifier");
        return std::nullopt;
    }
    return str;
}

std::string_view TextParser::readUntilCharacter(char c) {
    return readWhile([c](char currentC) { return currentC != c; });
}

std::optional<std::string_view> TextParser::readUntilCharacterAndParse(char c) {
    auto str = readUntilCharacter(c);
    if (!parse(c)) {
        return std::nullopt;
    }
    return str;
}

void TextParser::setErrorAtCurrentPosition(const Error& error) {
    _error = {ParseError(error, _position)};
}

void TextParser::setErrorAtCurrentPosition(std::string_view errorMessage) {
    setErrorAtCurrentPosition(Error(StringCache::getGlobal().makeString(errorMessage)));
}

void TextParser::prependError(std::string_view errorMessage) {
    SC_ASSERT(hasError());
    _error->error = _error->error.rethrow(StringCache::getGlobal().makeString(errorMessage));
}

Error TextParser::makeParseError(std::string_view str, std::string_view sourceErrorMessage, size_t position) {
    SC_ASSERT(position <= str.size());
    auto targetEndPosition = position + kMaxErrorMessageLength;
    auto resolvedEndPosition = std::min(str.size(), targetEndPosition);

    auto debugString = str.substr(position, resolvedEndPosition - position);

    std::string suffix;
    if (debugString.empty()) {
        suffix = "<EOF>";
    } else {
        suffix = debugString;
        if (debugString.size() != str.size() - position) {
            suffix += "…";
        }
    }

    return Error(STRING_FORMAT("At position {} ({}): {}", position, suffix, sourceErrorMessage));
}

} // namespace Valdi
