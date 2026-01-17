#include "valdi_protobuf/RepeatedField.hpp"
#include "valdi_protobuf/Field.hpp"
#include "valdi_protobuf/Message.hpp"

namespace Valdi::Protobuf {

RepeatedField::RepeatedField() = default;
RepeatedField::~RepeatedField() = default;

size_t RepeatedField::size() const {
    return _values.size();
}

bool RepeatedField::empty() const {
    return _values.empty();
}

void RepeatedField::reserve(size_t reserve) {
    _values.reserve(reserve);
}

void RepeatedField::clear() {
    _values.clear();
}

Ref<RepeatedField> RepeatedField::clone() const {
    auto out = makeShared<RepeatedField>();
    out->reserve(size());

    for (const auto& item : *this) {
        out->append(item.clone());
    }

    return out;
}

const Field& RepeatedField::last() const {
    return _values[_values.size() - 1];
}

Field& RepeatedField::last() {
    return _values[_values.size() - 1];
}

void RepeatedField::append(Field&& field) {
    _values.emplace_back(std::move(field));
}

void RepeatedField::append(const Field& field) {
    _values.emplace_back(field);
}

Field& RepeatedField::append() {
    return _values.emplace_back();
}

const Field* RepeatedField::begin() const {
    return &_values[0];
}

const Field* RepeatedField::end() const {
    return &_values[_values.size()];
}

Field* RepeatedField::begin() {
    return &_values[0];
}

Field* RepeatedField::end() {
    return &_values[_values.size()];
}

const Field& RepeatedField::operator[](size_t i) const {
    return _values[i];
}

Field& RepeatedField::operator[](size_t i) {
    return _values[i];
}

} // namespace Valdi::Protobuf
