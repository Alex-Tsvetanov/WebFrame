//
//  Parser.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/19/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <fmt/format.h>

namespace Valdi {

template<typename T>
class Parser {
public:
    Parser(const T* begin, const T* end) : _begin(begin), _end(end), _current(begin) {}

    template<typename Out>
    [[nodiscard]] Result<const Out*> parse(size_t requiredSize) {
        return consumeAndAdvance<Out>(requiredSize);
    }

    template<typename Out>
    [[nodiscard]] Result<const Out*> parseStruct() {
        return parse<Out>(sizeof(Out));
    }

    template<typename Out>
    [[nodiscard]] Result<Out> parseValue() {
        return parseStruct<Out>().template map<Out>([](const Out* value) { return *value; });
    }

    bool isAtEnd() {
        return _current == _end;
    }

    const T* getCurrent() const {
        return _current;
    }

    const T* getBegin() const {
        return _begin;
    }

    const T* getEnd() const {
        return _end;
    }

    void setEnd(const T* end) {
        _end = end;
    }

    size_t getDistanceToEnd() {
        return static_cast<size_t>(_end - _current);
    }

    size_t getDistanceToBegin() {
        return static_cast<size_t>(_current - _begin);
    }

private:
    const T* _begin;
    const T* _end;
    const T* _current;

    template<typename Out>
    [[nodiscard]] Result<const Out*> consumeAndAdvance(size_t requiredSize) {
        if (getDistanceToEnd() < requiredSize) {
            return Error(STRING_FORMAT("Unable to parse, missing at least {} bytes, only have {} bytes remaining",
                                       requiredSize,
                                       _end - _current));
        }
        const auto* current = _current;
        _current += requiredSize;

        return reinterpret_cast<const Out*>(current);
    }
};

} // namespace Valdi
