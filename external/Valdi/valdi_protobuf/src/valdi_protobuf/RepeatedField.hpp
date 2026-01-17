#pragma once

#include "valdi_protobuf/Field.hpp"
#include <vector>

namespace Valdi::Protobuf {

class RepeatedField : public SimpleRefCountable {
public:
    RepeatedField();
    ~RepeatedField() override;

    bool empty() const;
    size_t size() const;

    Ref<RepeatedField> clone() const;

    void reserve(size_t reserve);
    void clear();

    const Field& last() const;
    Field& last();
    void append(Field&& field);
    void append(const Field& field);
    Field& append();

    const Field& operator[](size_t i) const;
    Field& operator[](size_t i);

    const Field* begin() const;
    const Field* end() const;
    Field* begin();
    Field* end();

private:
    std::vector<Field> _values;
};

} // namespace Valdi::Protobuf
