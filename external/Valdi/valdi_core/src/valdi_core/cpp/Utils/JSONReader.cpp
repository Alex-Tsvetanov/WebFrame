//
//  JSONReader.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 12/21/23.
//  Copyright © 2023 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Utils/JSONReader.hpp"
#include <fmt/format.h>

namespace Valdi {

static inline std::string codePointToUTF8(unsigned int cp) {
    std::string result;

    // based on description from http://en.wikipedia.org/wiki/UTF-8

    if (cp <= 0x7f) {
        result.resize(1);
        result[0] = static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        result.resize(2);
        result[1] = static_cast<char>(0x80 | (0x3f & cp));
        result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
    } else if (cp <= 0xFFFF) {
        result.resize(3);
        result[2] = static_cast<char>(0x80 | (0x3f & cp));
        result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        result[0] = static_cast<char>(0xE0 | (0xf & (cp >> 12)));
    } else if (cp <= 0x10FFFF) {
        result.resize(4);
        result[3] = static_cast<char>(0x80 | (0x3f & cp));
        result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
        result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
    }

    return result;
}

JSONReader::JSONReader(std::string_view input) : _parser(input) {}

bool JSONReader::hasError() const {
    return _parser.hasError();
}

Error JSONReader::getError() const {
    return _parser.getError();
}

void JSONReader::setErrorAtCurrentPosition(std::string_view errorMessage) {
    _parser.setErrorAtCurrentPosition(errorMessage);
}

size_t JSONReader::position() const {
    return _parser.position();
}

size_t JSONReader::end() const {
    return _parser.end();
}

bool JSONReader::isAtEnd() const {
    return _parser.isAtEnd();
}

bool JSONReader::ensureIsAtEnd() {
    return _parser.ensureIsAtEnd();
}

bool JSONReader::ensureNotAtEnd() {
    return _parser.ensureNotAtEnd();
}

bool JSONReader::tryParseToken(char token) {
    if (!_parser.tryParse(token)) {
        return false;
    }

    _parser.tryParseWhitespaces();
    return true;
}

bool JSONReader::parseToken(char token) {
    if (!_parser.parse(token)) {
        return false;
    }

    _parser.tryParseWhitespaces();
    return true;
}

bool JSONReader::tryParseToken(std::string_view token) {
    if (!_parser.tryParse(token)) {
        return false;
    }

    _parser.tryParseWhitespaces();
    return true;
}

bool JSONReader::parseToken(std::string_view token) {
    if (!_parser.parse(token)) {
        return false;
    }

    _parser.tryParseWhitespaces();
    return true;
}

JSONReader::Token JSONReader::peekToken() {
    auto c = _parser.peek();
    switch (c) {
        case '{':
            return JSONReader::Token::Object;
        case '[':
            return JSONReader::Token::Array;
        case '"':
            return JSONReader::Token::String;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return JSONReader::Token::Number;
        case 't':
        case 'f':
            return JSONReader::Token::Boolean;
        case 'n':
            return JSONReader::Token::Null;
        default: {
            if (c == 0) {
                ensureNotAtEnd();
            } else {
                setErrorAtCurrentPosition(fmt::format("Invalid token '{}'", c));
            }
            return JSONReader::Token::Error;
        }
    }
}

bool JSONReader::tryParseEndObject() {
    return tryParseToken('}');
}

bool JSONReader::parseBeginObject() {
    return parseToken('{');
}

bool JSONReader::parseEndObject() {
    return parseToken('}');
}

bool JSONReader::tryParseEndArray() {
    return tryParseToken(']');
}

bool JSONReader::parseBeginArray() {
    return parseToken('[');
}

bool JSONReader::parseEndArray() {
    return parseToken(']');
}

bool JSONReader::parseComma() {
    return parseToken(',');
}

bool JSONReader::parseColon() {
    return parseToken(':');
}

bool JSONReader::parseString(std::string& output) {
    if (!_parser.parse('"')) {
        return false;
    }

    auto p = position();
    auto escaping = false;

    while (escaping || !_parser.tryParse('"')) {
        if (escaping) {
            escaping = false;
        } else {
            if (_parser.peek('\\')) {
                escaping = true;
            }
        }

        if (!_parser.skip()) {
            return false;
        }
    }

    return decodeString(_parser.substr(p, position() - 1), output);
}

bool JSONReader::decodeString(std::string_view str, std::string& decoded) {
    decoded.reserve(str.size());
    const auto* current = str.data();
    const auto* end = str.data() + str.size();
    while (current != end) {
        auto c = *current++;
        if (c == '"')
            break;
        if (c == '\\') {
            if (current == end) {
                setErrorAtCurrentPosition("Empty escape sequence in string");
                return false;
            }
            auto escape = *current++;
            switch (escape) {
                case '"':
                    decoded += '"';
                    break;
                case '/':
                    decoded += '/';
                    break;
                case '\\':
                    decoded += '\\';
                    break;
                case 'b':
                    decoded += '\b';
                    break;
                case 'f':
                    decoded += '\f';
                    break;
                case 'n':
                    decoded += '\n';
                    break;
                case 'r':
                    decoded += '\r';
                    break;
                case 't':
                    decoded += '\t';
                    break;
                case 'u': {
                    unsigned int unicode;
                    if (!decodeUnicodeCodePoint(current, end, unicode))
                        return false;
                    decoded += codePointToUTF8(unicode);
                } break;
                default:
                    setErrorAtCurrentPosition("Bad escape sequence in string");
                    return false;
            }
        } else {
            decoded += c;
        }
    }
    return true;
}

// Taken from the jsoncpp library
bool JSONReader::decodeUnicodeCodePoint(const char*& current, const char* end, unsigned int& unicode) {
    if (!decodeUnicodeEscapeSequence(current, end, unicode))
        return false;
    if (unicode >= 0xD800 && unicode <= 0xDBFF) {
        // surrogate pairs
        if (end - current < 6) {
            setErrorAtCurrentPosition("additional six characters expected to parse unicode surrogate pair.");
            return false;
        }
        if (*(current++) == '\\' && *(current++) == 'u') {
            unsigned int surrogatePair;
            if (decodeUnicodeEscapeSequence(current, end, surrogatePair)) {
                unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
            } else {
                return false;
            }
        } else {
            setErrorAtCurrentPosition(
                "expecting another \\u token to begin the second half of a unicode surrogate pair");
            return false;
        }
    }
    return true;
}

// Taken from the jsoncpp library
bool JSONReader::decodeUnicodeEscapeSequence(const char*& current, const char* end, unsigned int& retUnicode) {
    if (end - current < 4) {
        setErrorAtCurrentPosition("Bad unicode escape sequence in string: four digits expected.");
        return false;
    }
    int unicode = 0;
    for (int index = 0; index < 4; index++) {
        auto c = *current++;
        unicode *= 16;
        if (c >= '0' && c <= '9') {
            unicode += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            unicode += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            unicode += c - 'A' + 10;
        } else {
            setErrorAtCurrentPosition("Bad unicode escape sequence in string: hexadecimal digit expected.");
            return false;
        }
    }
    retUnicode = static_cast<unsigned int>(unicode);
    return true;
}

bool JSONReader::parseInt(int32_t& output) {
    if (!_parser.parseInt(output)) {
        return false;
    }
    _parser.tryParseWhitespaces();
    return true;
}

bool JSONReader::parseUInt(uint32_t& output) {
    if (!_parser.parseUInt(output)) {
        return false;
    }
    _parser.tryParseWhitespaces();
    return true;
}

bool JSONReader::parseDouble(double& output) {
    if (!_parser.parseDouble(output)) {
        return false;
    }
    _parser.tryParseWhitespaces();
    return true;
}

bool JSONReader::parseBool(bool& output) {
    if (tryParseToken("true")) {
        output = true;
        return true;
    } else if (tryParseToken("false")) {
        output = false;
        return true;
    } else {
        _parser.setErrorAtCurrentPosition("Expected boolean");
        return false;
    }
}

bool JSONReader::parseNull() {
    return parseToken("null");
}

} // namespace Valdi
