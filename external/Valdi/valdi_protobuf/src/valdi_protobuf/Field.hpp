#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StaticString.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"
#include "valdi_protobuf/FieldNumber.hpp"

namespace google::protobuf {
class FieldDescriptor;
} // namespace google::protobuf

namespace Valdi::Protobuf {

class Message;
class RepeatedField;

using EnumValue = uint32_t;

class Field {
public:
    enum class InternalType : uint8_t {
        Unset,
        Varint,
        Fixed64,
        Fixed32,
        Raw,
        Ref,
    };

    struct Raw {
        const Byte* data;
        size_t length;

        constexpr Raw(const Byte* data, size_t length) : data(data), length(length) {}

        std::string_view toStringView() const;
    };

    Field();
    ~Field();

    Field(const Field& other);
    Field(Field&& other) noexcept;

    Field& operator=(const Field& other);
    Field& operator=(Field&& other) noexcept;

    Field clone() const;

    size_t byteSize(FieldNumber fieldNumber, bool writeEvenIfEmpty, bool writeEvenIfEmptyForNestedFields) const;

    Byte* write(FieldNumber fieldNumber,
                bool writeEvenIfEmpty,
                bool writeEvenIfEmptyForNestedFields,
                Byte* bufferStart,
                Byte* bufferEnd) const;

    int32_t getInt32() const;
    void setInt32(int32_t v);

    int32_t getSInt32() const;
    void setSInt32(int32_t v);

    int64_t getInt64() const;
    void setInt64(int64_t v);

    int64_t getSInt64() const;
    void setSInt64(int64_t v);

    uint32_t getUInt32() const;
    void setUInt32(uint32_t v);

    uint64_t getUInt64() const;
    void setUInt64(uint64_t v);

    uint64_t getFixed64() const;
    void setFixed64(uint64_t v);

    uint32_t getFixed32() const;
    void setFixed32(uint32_t v);

    int64_t getSFixed64() const;
    void setSFixed64(int64_t v);

    int32_t getSFixed32() const;
    void setSFixed32(int32_t v);

    EnumValue getEnum() const;
    void setEnum(EnumValue v);

    bool getBool() const;
    void setBool(bool v);

    float getFloat() const;
    void setFloat(float v);

    double getDouble() const;
    void setDouble(double v);

    inline bool isUnset() const {
        return _type == InternalType::Unset;
    }

    void setIsOneOf(bool isOneOf);

    Raw getRaw() const;
    void setRaw(const Raw& raw);

    void append(Field field);
    Field& append();

    InternalType getInternalType() const;

    Message* getMessage() const;
    void setMessage(Message* message);

    /**
     Convert  the Field into a Repeated field if it isn't already, return the underlying
     RepeatedField object
     */
    RepeatedField* toRepeated();

    RepeatedField* toVarintRepeated();
    RepeatedField* toFixed64Repeated();
    RepeatedField* toFixed32Repeated();

    RepeatedField* toRepeated(const google::protobuf::FieldDescriptor& fieldDescriptor);

    const RepeatedField* getRepeated() const;
    RepeatedField* getRepeated();
    void setRepeated(RepeatedField* repeated);

    StaticString* getString() const;
    void setString(StaticString* string);

    ValueTypedArray* getTypedArray() const;
    void setTypedArray(ValueTypedArray* typedArray);

    static Field varint(uint64_t varint);
    static Field fixed64(uint64_t fixed64);
    static Field fixed32(uint32_t fixed32);
    static Field raw(const Byte* data, uint32_t length);
    static Field ref(RefCountable* ref);
    static Field message(const Ref<Message>& message);
    static Field repeated(const Ref<RepeatedField>& repeated);
    static Field string(const Ref<StaticString>& string);
    static Field typedArray(const Ref<ValueTypedArray>& typedArray);

private:
    union FieldStorage {
        uint64_t varint;
        uint64_t fixed64;
        uint32_t fixed32;
        const Byte* raw;
        RefCountable* ref;
    };
    FieldStorage _data;
    InternalType _type = InternalType::Unset;
    bool _isOneOf = false;
    uint32_t _rawLength = 0;

    Field(const FieldStorage& storage, InternalType type, uint32_t rawLength);

    void set(const FieldStorage& storage, InternalType type, uint32_t rawLength);

    void setVarint(uint64_t varint);
    uint64_t getVarint() const;

    void setRef(RefCountable* ref);

    template<typename T>
    T* getRefPtr() {
        if (_type == InternalType::Ref) {
            return dynamic_cast<T*>(_data.ref);
        }
        return nullptr;
    }

    template<typename T>
    T* getRefPtr() const {
        if (_type == InternalType::Ref) {
            return dynamic_cast<T*>(_data.ref);
        }
        return nullptr;
    }
};

} // namespace Valdi::Protobuf
