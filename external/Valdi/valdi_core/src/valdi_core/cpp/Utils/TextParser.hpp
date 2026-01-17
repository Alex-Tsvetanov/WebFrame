//
//  TextParser.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 1/20/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Error.hpp"
#include <optional>
#include <string>

namespace Valdi {

/**
 The TextParser is a helper class to facilitate parsing a text protocol from a string.
 It makes sure that parsing doesn't go outside of the string bounds, and emits human
 readable errors on parse failures.
 */
class TextParser {
public:
    explicit TextParser(std::string_view str);
    ~TextParser();

    bool hasError() const;
    Error getError() const;

    size_t position() const;
    size_t end() const;

    bool isAtEnd() const;
    bool ensureIsAtEnd();
    bool ensureNotAtEnd();

    std::string_view substr(size_t from, size_t to) const;

    bool tryParse(char c);
    bool parse(char c);
    bool peek(char c) const;
    char peek() const;

    bool tryParse(std::string_view term);
    bool parse(std::string_view term);

    bool skip();
    bool skipCount(size_t count);

    bool tryParseWhitespaces();

    std::optional<double> parseDouble();
    std::optional<int> parseInt();
    std::optional<uint32_t> parseUInt();
    std::optional<long long> parseHexLong();
    std::optional<std::string_view> parseIdentifier();
    bool parseDouble(double& output);
    bool parseInt(int& output);
    bool parseUInt(uint32_t& output);
    bool parseHexLong(long long& output);
    bool parseIdentifier(std::string_view& output);

    /**
     Read the string while the given predicate is true.
     Returns a string containing the read characters.
     */
    template<typename Pred>
    std::string_view readWhile(Pred&& pred) {
        auto start = position();
        while (tryParsePredicate(pred)) {
        }
        return substr(start, position());
    }

    /**
     Read the string until the given character is found or the end of the buffer is reached.
    Parser will be positioned right after the character was found.
     */
    std::string_view readUntilCharacter(char c);

    /**
     Read the string until the given character is found.
     Return the string read before reaching the character, or a null optional
     if the character was not found. Parser will be positioned right after the
     character was found.
     */
    std::optional<std::string_view> readUntilCharacterAndParse(char c);

    template<typename Pred>
    inline bool tryParsePredicate(Pred&& pred) {
        if (isAtEnd()) {
            return false;
        }

        if (pred(_str[_position])) {
            advance();
            return true;
        } else {
            return false;
        }
    }

    template<typename Pred>
    inline bool peekPredicate(Pred&& pred) const {
        return pred(peek());
    }

    void setErrorAtCurrentPosition(const Error& error);
    void setErrorAtCurrentPosition(std::string_view errorMessage);
    void prependError(std::string_view errorMessage);

    static Error makeParseError(std::string_view str, std::string_view sourceErrorMessage, size_t position);

private:
    struct ParseError {
        Error error;
        size_t position;
        inline ParseError(const Error& error, size_t position) : error(error), position(position) {}
    };

    std::string_view _str;
    size_t _position = 0;
    std::optional<ParseError> _error;

    void advance();
};

} // namespace Valdi
