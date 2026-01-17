//
//  CharactersIterator.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/3/23.
//

#pragma once

#include "snap_drawing/cpp/Text/Character.hpp"
#include <cstddef>

namespace snap::drawing {

/**
 A Helper to iterate through characters and process them
 by range.
 */
class CharactersIterator {
public:
    constexpr CharactersIterator(const Character* characters, size_t length)
        : CharactersIterator(characters, 0, length) {}
    constexpr CharactersIterator(const Character* characters, size_t start, size_t end)
        : _characters(characters), _end(end), _position(start), _rangeStart(start) {}

    constexpr bool isAtEnd() const {
        return _position == _end;
    }

    constexpr Character current() const {
        return _characters[_position];
    }

    constexpr void advance() {
        _position++;
    }

    constexpr size_t position() const {
        return _position;
    }

    constexpr void resetRange() {
        _rangeStart = _position;
    }

    constexpr bool hasRange() const {
        return _rangeStart != _position;
    }

    constexpr size_t rangeStart() const {
        return _rangeStart;
    }

    constexpr size_t rangeEnd() const {
        return _position;
    }

    constexpr size_t rangeLength() const {
        return _position - _rangeStart;
    }

private:
    const Character* _characters;
    size_t _end;
    size_t _position;
    size_t _rangeStart;
};

} // namespace snap::drawing
