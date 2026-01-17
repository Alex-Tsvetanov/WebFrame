// Copyright © 2024 Snap, Inc. All rights reserved.

#pragma once

namespace Valdi::Protobuf {

class Field;

class RepeatedFieldIterator {
public:
    inline RepeatedFieldIterator(Field* begin, Field* end) : _begin(begin), _end(end) {}

    inline Field* begin() const {
        return _begin;
    }

    inline Field* end() const {
        return _end;
    }

    inline bool empty() const {
        return _begin == _end;
    }

private:
    Field* _begin;
    Field* _end;
};

class RepeatedFieldConstIterator {
public:
    inline RepeatedFieldConstIterator(const Field* begin, const Field* end) : _begin(begin), _end(end) {}

    inline const Field* begin() const {
        return _begin;
    }

    inline const Field* end() const {
        return _end;
    }

    inline bool empty() const {
        return _begin == _end;
    }

private:
    const Field* _begin;
    const Field* _end;
};

} // namespace Valdi::Protobuf
