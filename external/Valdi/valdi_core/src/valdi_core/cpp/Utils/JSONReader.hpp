//
//  JSONReader.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 12/21/23.
//  Copyright © 2023 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/TextParser.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

class JSONReader {
public:
    enum class Token {
        Error,
        Object,
        Array,
        String,
        Number,
        Boolean,
        Null,
    };

    JSONReader(std::string_view input);

    bool hasError() const;
    Error getError() const;
    void setErrorAtCurrentPosition(std::string_view errorMessage);

    size_t position() const;
    size_t end() const;

    bool isAtEnd() const;
    bool ensureIsAtEnd();
    bool ensureNotAtEnd();

    Token peekToken();

    bool parseBeginObject();
    bool parseEndObject();
    bool tryParseEndObject();

    bool parseBeginArray();
    bool parseEndArray();
    bool tryParseEndArray();

    bool parseComma();
    bool parseColon();

    bool parseString(std::string& output);

    bool parseInt(int32_t& output);
    bool parseUInt(uint32_t& output);

    bool parseDouble(double& output);

    bool parseBool(bool& output);

    bool parseNull();

private:
    TextParser _parser;
    std::string _tmp;

    bool tryParseToken(char token);
    bool parseToken(char token);
    bool tryParseToken(std::string_view token);
    bool parseToken(std::string_view token);

    bool decodeString(std::string_view str, std::string& decoded);
    bool decodeUnicodeCodePoint(const char*& current, const char* end, unsigned int& unicode);
    bool decodeUnicodeEscapeSequence(const char*& current, const char* end, unsigned int& retUnicode);
};

} // namespace Valdi
