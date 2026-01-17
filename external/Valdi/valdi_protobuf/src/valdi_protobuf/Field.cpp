#include "valdi_protobuf/Field.hpp"
#include "valdi_protobuf/Message.hpp"
#include "valdi_protobuf/RepeatedField.hpp"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

namespace Valdi::Protobuf {

using WireFormat = google::protobuf::internal::WireFormatLite;
using CodedOutputStream = google::protobuf::io::CodedOutputStream;

std::string_view Field::Raw::toStringView() const {
    return std::string_view(reinterpret_cast<const char*>(data), static_cast<size_t>(length));
}

enum class PackedRepeatedType {
    PackedRepeatedTypeUnpacked,
    PackedRepeatedTypeVarint,
    PackedRepeatedTypeFixed64,
    PackedRepeatedTypeFixed32,
};

// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)

Field::Field() {
    _data.varint = 0;
}

Field::Field(const FieldStorage& storage, InternalType type, uint32_t rawLength)
    : _data(storage), _type(type), _rawLength(rawLength) {}

Field::~Field() {
    set(FieldStorage(), InternalType::Unset, 0);
}

Field::Field(const Field& other) {
    set(other._data, other._type, other._rawLength);
}

Field::Field(Field&& other) noexcept : _data(other._data), _type(other._type), _rawLength(other._rawLength) {
    other._type = InternalType::Unset;
    other._data.varint = 0;
    other._rawLength = 0;
}

Field& Field::operator=(const Field& other) {
    if (this != &other) {
        set(other._data, other._type, other._rawLength);
    }
    return *this;
}

Field& Field::operator=(Field&& other) noexcept {
    if (this != &other) {
        if (_type == InternalType::Ref) {
            unsafeRelease(_data.ref);
        }

        _data = other._data;
        _type = other._type;
        _rawLength = other._rawLength;

        other._type = InternalType::Unset;
        other._data.varint = 0;
        other._rawLength = 0;
    }
    return *this;
}

Field Field::clone() const {
    switch (_type) {
        case InternalType::Unset:
        case InternalType::Varint:
        case InternalType::Fixed64:
        case InternalType::Fixed32:
        case InternalType::Raw:
            return *this;
        case InternalType::Ref: {
            const auto* message = getMessage();
            if (message != nullptr) {
                return Field::message(message->clone());
            }
            const auto* repeated = getRepeated();
            if (repeated != nullptr) {
                return Field::repeated(repeated->clone());
            }

            return *this;
        }
    }
}

static size_t computeByteSize(FieldNumber fieldNumber, size_t length, WireFormat::WireType format) {
    auto fieldNumberInt = static_cast<int>(fieldNumber);
    return CodedOutputStream::VarintSize32(WireFormat::MakeTag(fieldNumberInt, format)) + length;
}

static size_t getLengthDelimitedByteSize(FieldNumber fieldNumber, size_t length, bool writeEvenIfEmpty) {
    if (!writeEvenIfEmpty && length == 0) {
        return 0;
    }

    return computeByteSize(fieldNumber,
                           CodedOutputStream::VarintSize32(static_cast<uint32_t>(length)) + length,
                           WireFormat::WIRETYPE_LENGTH_DELIMITED);
}

static PackedRepeatedType getPackedTypeForType(Field::InternalType type) {
    switch (type) {
        case Field::InternalType::Unset:
            return PackedRepeatedType::PackedRepeatedTypeUnpacked;
        case Field::InternalType::Varint:
            return PackedRepeatedType::PackedRepeatedTypeVarint;
        case Field::InternalType::Fixed64:
            return PackedRepeatedType::PackedRepeatedTypeFixed64;
        case Field::InternalType::Fixed32:
            return PackedRepeatedType::PackedRepeatedTypeFixed32;
        case Field::InternalType::Raw:
            return PackedRepeatedType::PackedRepeatedTypeUnpacked;
        case Field::InternalType::Ref:
            return PackedRepeatedType::PackedRepeatedTypeUnpacked;
    }
}

static PackedRepeatedType resolvePackedRepeatedType(const RepeatedField& repeated) {
    const auto* it = repeated.begin();
    const auto* end = repeated.end();

    if (it == end) {
        return PackedRepeatedType::PackedRepeatedTypeUnpacked;
    }

    PackedRepeatedType packedType = getPackedTypeForType(it->getInternalType());

    if (packedType != PackedRepeatedType::PackedRepeatedTypeUnpacked) {
        it++;

        while (it != end) {
            auto newPackedType = getPackedTypeForType(it->getInternalType());
            if (newPackedType != packedType) {
                return PackedRepeatedType::PackedRepeatedTypeUnpacked;
            }
            it++;
        }
    }

    return packedType;
}

size_t Field::byteSize(FieldNumber fieldNumber, bool writeEvenIfEmpty, bool writeEvenIfEmptyForNestedFields) const {
    writeEvenIfEmpty |= _isOneOf;

    switch (_type) {
        case InternalType::Unset:
            return 0;
        case InternalType::Varint: {
            auto varint = _data.varint;
            if (!writeEvenIfEmpty && varint == 0) {
                return 0;
            }

            return computeByteSize(fieldNumber, CodedOutputStream::VarintSize64(varint), WireFormat::WIRETYPE_VARINT);
        }
        case InternalType::Fixed64: {
            auto i64 = _data.fixed64;
            if (!writeEvenIfEmpty && i64 == 0) {
                return 0;
            }
            return computeByteSize(fieldNumber, sizeof(int64_t), WireFormat::WIRETYPE_FIXED64);
        }
        case InternalType::Fixed32: {
            auto i32 = _data.fixed32;
            if (!writeEvenIfEmpty && i32 == 0) {
                return 0;
            }
            return computeByteSize(fieldNumber, sizeof(int32_t), WireFormat::WIRETYPE_FIXED32);
        }
        case InternalType::Raw:
            return getLengthDelimitedByteSize(fieldNumber, _rawLength, true);
        case InternalType::Ref: {
            auto* message = getMessage();
            if (message != nullptr) {
                return getLengthDelimitedByteSize(
                    fieldNumber, message->encodedByteSize(writeEvenIfEmptyForNestedFields), true);
            }

            const auto* repeated = getRepeated();
            if (repeated != nullptr) {
                auto packedType = resolvePackedRepeatedType(*repeated);

                switch (packedType) {
                    case PackedRepeatedType::PackedRepeatedTypeUnpacked: {
                        size_t byteSize = 0;
                        for (const auto& value : *repeated) {
                            byteSize += value.byteSize(fieldNumber, true, writeEvenIfEmptyForNestedFields);
                        }
                        return byteSize;
                    }
                    case PackedRepeatedType::PackedRepeatedTypeVarint: {
                        int byteSize = 0;
                        for (const auto& value : *repeated) {
                            byteSize += CodedOutputStream::VarintSize64(value._data.varint);
                        }
                        return getLengthDelimitedByteSize(fieldNumber, static_cast<size_t>(byteSize), true);
                    } break;
                    case PackedRepeatedType::PackedRepeatedTypeFixed64:
                        return getLengthDelimitedByteSize(fieldNumber, sizeof(int64_t) * repeated->size(), true);
                    case PackedRepeatedType::PackedRepeatedTypeFixed32:
                        return getLengthDelimitedByteSize(fieldNumber, sizeof(int32_t) * repeated->size(), true);
                }
            }

            const auto* string = getString();
            if (string != nullptr) {
                return getLengthDelimitedByteSize(fieldNumber, string->utf8Storage().length, writeEvenIfEmpty);
            }

            const auto* typedArray = getTypedArray();
            if (typedArray != nullptr) {
                return getLengthDelimitedByteSize(fieldNumber, typedArray->getBuffer().size(), writeEvenIfEmpty);
            }

            return 0;
        }
    }
}

static Byte* writeTag(FieldNumber fieldNumber, WireFormat::WireType format, Byte* bufferStart, Byte* /*bufferEnd*/) {
    return WireFormat::WriteTagToArray(static_cast<int>(fieldNumber), format, bufferStart);
}

template<typename F>
static Byte* writeLengthDelimited(
    FieldNumber fieldNumber, size_t length, bool writeEvenIfEmpty, Byte* bufferStart, Byte* bufferEnd, F&& write) {
    if (!writeEvenIfEmpty && length == 0) {
        return bufferStart;
    }

    bufferStart = writeTag(fieldNumber, WireFormat::WIRETYPE_LENGTH_DELIMITED, bufferStart, bufferEnd);
    bufferStart = CodedOutputStream::WriteVarint32ToArray(static_cast<uint32_t>(length), bufferStart);

    auto* endPtr = bufferStart + length;

    SC_ABORT_UNLESS(endPtr <= bufferEnd, "Out of bounds");

    write(bufferStart);

    return endPtr;
}

Byte* Field::write(FieldNumber fieldNumber,
                   bool writeEvenIfEmpty,
                   bool writeEvenIfEmptyForNestedFields,
                   Byte* bufferStart,
                   Byte* bufferEnd) const {
    writeEvenIfEmpty |= _isOneOf;

    switch (_type) {
        case InternalType::Unset:
            return bufferStart;
        case InternalType::Varint: {
            auto varint = _data.varint;
            if (!writeEvenIfEmpty && varint == 0) {
                return bufferStart;
            }

            bufferStart = writeTag(fieldNumber, WireFormat::WIRETYPE_VARINT, bufferStart, bufferEnd);

            return CodedOutputStream::WriteVarint64ToArray(varint, bufferStart);
        }
        case InternalType::Fixed64: {
            auto i64 = _data.fixed64;
            if (!writeEvenIfEmpty && i64 == 0) {
                return bufferStart;
            }

            return WireFormat::WriteFixed64ToArray(static_cast<int>(fieldNumber), i64, bufferStart);
        }
        case InternalType::Fixed32: {
            auto i32 = _data.fixed32;
            if (!writeEvenIfEmpty && i32 == 0) {
                return bufferStart;
            }

            return WireFormat::WriteFixed32ToArray(static_cast<int>(fieldNumber), i32, bufferStart);
        }
        case InternalType::Raw: {
            auto rawLength = static_cast<size_t>(_rawLength);
            return writeLengthDelimited(fieldNumber, rawLength, true, bufferStart, bufferEnd, [&](auto* buffer) {
                std::memcpy(buffer, _data.raw, rawLength);
            });
        }
        case InternalType::Ref: {
            const auto* message = getMessage();
            if (message != nullptr) {
                return writeLengthDelimited(
                    fieldNumber, message->getCachedEncodedByteSize(), true, bufferStart, bufferEnd, [&](auto* buffer) {
                        message->encode(writeEvenIfEmptyForNestedFields, buffer, bufferEnd);
                    });
            }

            const auto* repeated = getRepeated();
            if (repeated != nullptr) {
                auto packedType = resolvePackedRepeatedType(*repeated);

                switch (packedType) {
                    case PackedRepeatedType::PackedRepeatedTypeUnpacked: {
                        for (const auto& value : *repeated) {
                            bufferStart =
                                value.write(fieldNumber, true, writeEvenIfEmptyForNestedFields, bufferStart, bufferEnd);
                        }

                        return bufferStart;
                    }
                    case PackedRepeatedType::PackedRepeatedTypeVarint: {
                        int byteSize = 0;
                        for (const auto& value : *repeated) {
                            byteSize += CodedOutputStream::VarintSize64(value._data.varint);
                        }

                        return writeLengthDelimited(fieldNumber,
                                                    static_cast<size_t>(byteSize),
                                                    true,
                                                    bufferStart,
                                                    bufferEnd,
                                                    [&](auto* buffer) {
                                                        for (const auto& value : *repeated) {
                                                            buffer = CodedOutputStream::WriteVarint64ToArray(
                                                                value._data.varint, buffer);
                                                        }
                                                    });
                    } break;
                    case PackedRepeatedType::PackedRepeatedTypeFixed64:
                        return writeLengthDelimited(fieldNumber,
                                                    sizeof(int64_t) * repeated->size(),
                                                    true,
                                                    bufferStart,
                                                    bufferEnd,
                                                    [&](auto* buffer) {
                                                        for (const auto& value : *repeated) {
                                                            buffer = WireFormat::WriteFixed64NoTagToArray(
                                                                value._data.fixed64, buffer);
                                                        }
                                                    });
                    case PackedRepeatedType::PackedRepeatedTypeFixed32:
                        return writeLengthDelimited(fieldNumber,
                                                    sizeof(int32_t) * repeated->size(),
                                                    true,
                                                    bufferStart,
                                                    bufferEnd,
                                                    [&](auto* buffer) {
                                                        for (const auto& value : *repeated) {
                                                            buffer = WireFormat::WriteFixed32NoTagToArray(
                                                                value._data.fixed32, buffer);
                                                        }
                                                    });
                }
            }

            const auto* string = getString();
            if (string != nullptr) {
                auto utf8Storage = string->utf8Storage();

                return writeLengthDelimited(
                    fieldNumber, utf8Storage.length, writeEvenIfEmpty, bufferStart, bufferEnd, [&](auto* buffer) {
                        std::memcpy(buffer, utf8Storage.data, utf8Storage.length);
                    });
            }

            const auto* typedArray = getTypedArray();
            if (typedArray != nullptr) {
                return writeLengthDelimited(
                    fieldNumber,
                    typedArray->getBuffer().size(),
                    writeEvenIfEmpty,
                    bufferStart,
                    bufferEnd,
                    [&](auto* buffer) {
                        std::memcpy(buffer, typedArray->getBuffer().data(), typedArray->getBuffer().size());
                    });
            }

            return bufferStart;
        }
    }
}

void Field::setIsOneOf(bool isOneOf) {
    _isOneOf = isOneOf;
}

void Field::setVarint(uint64_t varint) {
    FieldStorage storage;
    storage.varint = varint;

    set(storage, InternalType::Varint, 0);
}

uint64_t Field::getVarint() const {
    switch (_type) {
        case InternalType::Varint:
            return _data.varint;
        case InternalType::Fixed64:
            return _data.fixed64;
        case InternalType::Fixed32:
            return static_cast<uint64_t>(_data.fixed32);
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

int32_t Field::getInt32() const {
    switch (_type) {
        case InternalType::Varint:
            return static_cast<int32_t>(_data.varint);
        case InternalType::Fixed64:
            return static_cast<int32_t>(_data.fixed64);
        case InternalType::Fixed32:
            return static_cast<int32_t>(_data.fixed32);
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setInt32(int32_t v) {
    setVarint(static_cast<uint64_t>(v));
}

int32_t Field::getSInt32() const {
    switch (_type) {
        case InternalType::Varint:
            return WireFormat::ZigZagDecode32(static_cast<uint32_t>(_data.varint));
        case InternalType::Fixed64:
            return static_cast<int32_t>(_data.fixed64);
        case InternalType::Fixed32:
            return static_cast<int32_t>(_data.fixed32);
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setSInt32(int32_t v) {
    setVarint(WireFormat::ZigZagEncode64(static_cast<int64_t>(v)));
}

int64_t Field::getInt64() const {
    return static_cast<int64_t>(getVarint());
}

void Field::setInt64(int64_t v) {
    setVarint(static_cast<int64_t>(v));
}

int64_t Field::getSInt64() const {
    switch (_type) {
        case InternalType::Varint:
            return WireFormat::ZigZagDecode64(_data.varint);
        case InternalType::Fixed64:
            return static_cast<int64_t>(_data.fixed64);
        case InternalType::Fixed32:
            return static_cast<int64_t>(_data.fixed32);
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setSInt64(int64_t v) {
    setVarint(WireFormat::ZigZagEncode64(v));
}

uint32_t Field::getUInt32() const {
    switch (_type) {
        case InternalType::Varint:
            return static_cast<uint32_t>(_data.varint);
        case InternalType::Fixed64:
            return static_cast<uint32_t>(_data.fixed64);
        case InternalType::Fixed32:
            return _data.fixed32;
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setUInt32(uint32_t v) {
    setVarint(v);
}

uint64_t Field::getUInt64() const {
    return getVarint();
}

void Field::setUInt64(uint64_t v) {
    setVarint(v);
}

uint32_t Field::getFixed32() const {
    switch (_type) {
        case InternalType::Varint:
            return static_cast<uint32_t>(_data.varint);
        case InternalType::Fixed64:
            return static_cast<uint32_t>(_data.fixed64);
        case InternalType::Fixed32:
            return _data.fixed32;
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setFixed32(uint32_t v) {
    FieldStorage storage;
    storage.fixed32 = v;
    set(storage, InternalType::Fixed32, 0);
}

int32_t Field::getSFixed32() const {
    return static_cast<int32_t>(getFixed32());
}

void Field::setSFixed32(int32_t v) {
    setFixed32(static_cast<uint32_t>(v));
}

uint64_t Field::getFixed64() const {
    switch (_type) {
        case InternalType::Varint:
            return _data.varint;
        case InternalType::Fixed64:
            return _data.fixed64;
        case InternalType::Fixed32:
            return static_cast<uint64_t>(_data.fixed32);
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setFixed64(uint64_t v) {
    FieldStorage storage;
    storage.fixed64 = v;
    set(storage, InternalType::Fixed64, 0);
}

int64_t Field::getSFixed64() const {
    return static_cast<int64_t>(getFixed64());
}

void Field::setSFixed64(int64_t v) {
    setFixed64(static_cast<uint64_t>(v));
}

bool Field::getBool() const {
    switch (_type) {
        case InternalType::Varint:
            return _data.varint != 0;
        case InternalType::Fixed64:
            return _data.fixed64 != 0;
        case InternalType::Fixed32:
            return _data.fixed32 != 0;
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return false;
    }
}

void Field::setBool(bool v) {
    setVarint(v ? 1 : 0);
}

float Field::getFloat() const {
    switch (_type) {
        case InternalType::Varint:
            return static_cast<float>(WireFormat::DecodeDouble(_data.varint));
        case InternalType::Fixed64:
            return static_cast<float>(WireFormat::DecodeDouble(_data.fixed64));
        case InternalType::Fixed32:
            return WireFormat::DecodeFloat(_data.fixed32);
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setFloat(float v) {
    setFixed32(WireFormat::EncodeFloat(v));
}

double Field::getDouble() const {
    switch (_type) {
        case InternalType::Varint:
            return WireFormat::DecodeDouble(_data.varint);
        case InternalType::Fixed64:
            return WireFormat::DecodeDouble(_data.fixed64);
        case InternalType::Fixed32:
            return static_cast<double>(WireFormat::DecodeFloat(_data.fixed32));
        case InternalType::Unset:
        case InternalType::Raw:
        case InternalType::Ref:
            return 0;
    }
}

void Field::setDouble(double v) {
    setFixed64(WireFormat::EncodeDouble(v));
}

EnumValue Field::getEnum() const {
    return static_cast<EnumValue>(getVarint());
}

void Field::setEnum(EnumValue v) {
    setVarint(static_cast<uint64_t>(v));
}

Field::Raw Field::getRaw() const {
    switch (_type) {
        case InternalType::Raw:
            return Field::Raw(_data.raw, _rawLength);
        case InternalType::Varint:
        case InternalType::Fixed64:
        case InternalType::Fixed32:
        case InternalType::Unset:
        case InternalType::Ref:
            return Field::Raw(nullptr, 0);
    }
}

void Field::setRaw(const Field::Raw& raw) {
    FieldStorage storage;
    storage.raw = raw.data;
    set(storage, InternalType::Raw, static_cast<uint32_t>(raw.length));
}

void Field::append(Field field) {
    toRepeated()->append(std::move(field));
}

Field& Field::append() {
    return toRepeated()->append();
}

Field::InternalType Field::getInternalType() const {
    return _type;
}

Message* Field::getMessage() const {
    if (_type == InternalType::Ref) {
        return dynamic_cast<Message*>(_data.ref);
    }
    return nullptr;
}

void Field::setRef(RefCountable* ref) {
    FieldStorage storage;
    storage.ref = ref;
    set(storage, InternalType::Ref, 0);
}

void Field::setMessage(Message* message) {
    setRef(message);
}

RepeatedField* Field::toRepeated() {
    auto* repeated = getRefPtr<RepeatedField>();
    if (repeated == nullptr) {
        // Convert the field into a repeated field with a single value
        auto repeatedField = makeShared<RepeatedField>();
        repeated = repeatedField.get();
        if (!isUnset()) {
            repeatedField->append(std::move(*this));
        }
        setRepeated(repeatedField.get());
    }

    return repeated;
}

RepeatedField* Field::toRepeated(const google::protobuf::FieldDescriptor& fieldDescriptor) {
    switch (fieldDescriptor.type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            return toFixed64Repeated();
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            return toFixed32Repeated();
        case google::protobuf::FieldDescriptor::TYPE_INT64:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_INT32:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            return toFixed64Repeated();
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            return toFixed32Repeated();
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            return toRepeated();
        case google::protobuf::FieldDescriptor::TYPE_GROUP:
            return toRepeated();
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            return toRepeated();
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
            return toRepeated();
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            return toFixed32Repeated();
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            return toFixed64Repeated();
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            return toVarintRepeated();
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
            return toVarintRepeated();
    }
}

using RepeatedParser = void (*)(RepeatedField* repeated, const Byte* data, size_t length);

static RepeatedField* parsePackedRepeated(Field* field, const Byte* data, size_t length, RepeatedParser parser) {
    auto repeatedField = makeShared<RepeatedField>();
    parser(repeatedField.get(), data, length);
    field->setRepeated(repeatedField.get());
    return repeatedField.get();
}

static RepeatedField* toPackedRepeated(Field* field, RepeatedParser parser) {
    if (field->getInternalType() == Field::InternalType::Raw) {
        auto raw = field->getRaw();
        return parsePackedRepeated(field, raw.data, raw.length, parser);
    } else {
        auto* typedArray = field->getTypedArray();
        if (typedArray != nullptr) {
            return parsePackedRepeated(field, typedArray->getBuffer().data(), typedArray->getBuffer().size(), parser);
        }
    }

    return field->toRepeated();
}

static void parseVarintRepeated(RepeatedField* repeated, const Byte* data, size_t length) {
    google::protobuf::io::CodedInputStream inputStream(data, static_cast<int>(length));

    uint64_t varint;
    while (inputStream.ReadVarint64(&varint)) {
        repeated->append(Field::varint(varint));
    }
}

static void parseFixed64Repeated(RepeatedField* repeated, const Byte* data, size_t length) {
    google::protobuf::io::CodedInputStream inputStream(data, static_cast<int>(length));

    uint64_t value;
    while (inputStream.ReadLittleEndian64(&value)) {
        repeated->append(Field::fixed64(value));
    }
}

static void parseFixed32Repeated(RepeatedField* repeated, const Byte* data, size_t length) {
    google::protobuf::io::CodedInputStream inputStream(data, static_cast<int>(length));

    uint32_t value;
    while (inputStream.ReadLittleEndian32(&value)) {
        repeated->append(Field::fixed32(value));
    }
}

RepeatedField* Field::toVarintRepeated() {
    return toPackedRepeated(this, parseVarintRepeated);
}

RepeatedField* Field::toFixed32Repeated() {
    return toPackedRepeated(this, parseFixed32Repeated);
}

RepeatedField* Field::toFixed64Repeated() {
    return toPackedRepeated(this, parseFixed64Repeated);
}

const RepeatedField* Field::getRepeated() const {
    return getRefPtr<RepeatedField>();
}

RepeatedField* Field::getRepeated() {
    return getRefPtr<RepeatedField>();
}

void Field::setRepeated(RepeatedField* repeated) {
    setRef(repeated);
}

StaticString* Field::getString() const {
    return getRefPtr<StaticString>();
}

void Field::setString(StaticString* string) {
    setRef(string);
}

ValueTypedArray* Field::getTypedArray() const {
    return getRefPtr<ValueTypedArray>();
}

void Field::setTypedArray(ValueTypedArray* typedArray) {
    setRef(typedArray);
}

void Field::set(const FieldStorage& storage, InternalType type, uint32_t rawLength) {
    if (_type == InternalType::Ref) {
        unsafeRelease(_data.ref);
    }

    _data = storage;
    _type = type;
    _rawLength = rawLength;

    if (type == InternalType::Ref) {
        unsafeRetain(storage.ref);
    }
}

Field Field::varint(uint64_t varint) {
    Field::FieldStorage storage;
    storage.varint = varint;
    return Field(storage, InternalType::Varint, 0);
}

Field Field::fixed64(uint64_t fixed64) {
    Field::FieldStorage storage;
    storage.fixed64 = fixed64;
    return Field(storage, InternalType::Fixed64, 0);
}

Field Field::fixed32(uint32_t fixed32) {
    Field::FieldStorage storage;
    storage.fixed32 = fixed32;
    return Field(storage, InternalType::Fixed32, 0);
}

Field Field::raw(const Byte* data, uint32_t length) {
    Field::FieldStorage storage;
    storage.raw = data;
    return Field(storage, InternalType::Raw, length);
}

Field Field::ref(RefCountable* ref) {
    Field::FieldStorage storage;
    storage.ref = unsafeRetain(ref);
    return Field(storage, InternalType::Ref, 0);
}

Field Field::message(const Ref<Message>& message) {
    return ref(message.get());
}

Field Field::repeated(const Ref<RepeatedField>& repeated) {
    return ref(repeated.get());
}

Field Field::string(const Ref<StaticString>& string) {
    return ref(string.get());
}

Field Field::typedArray(const Ref<ValueTypedArray>& typedArray) {
    return ref(typedArray.get());
}

// NOLINTEND(cppcoreguidelines-pro-type-union-access)

} // namespace Valdi::Protobuf
