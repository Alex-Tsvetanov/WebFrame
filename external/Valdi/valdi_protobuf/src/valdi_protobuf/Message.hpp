#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/ExceptionTracker.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_protobuf/Field.hpp"
#include "valdi_protobuf/FieldMap.hpp"
#include "valdi_protobuf/FieldNumber.hpp"
#include "valdi_protobuf/RepeatedField.hpp"
#include "valdi_protobuf/RepeatedFieldIterator.hpp"

#include <vector>

namespace Valdi {
class ILogger;
}

namespace google::protobuf {
class Descriptor;
class FieldDescriptor;
} // namespace google::protobuf

namespace Valdi::Protobuf {

class IMessageFactory;

struct JSONPrintOptions {
    // Whether to add spaces, line breaks and indentation to make the JSON output
    // easy to read.
    bool pretty = false;
    // Whether to always print primitive fields. By default proto3 primitive
    // fields with default values will be omitted in JSON output. For example, an
    // int32 field set to 0 will be omitted. Set this flag to true will override
    // the default behavior and print primitive fields regardless of their values.
    bool alwaysPrintPrimitiveFields = false;
    // Whether to always print enums as ints. By default they are rendered as
    // strings.
    bool alwaysPrintEnumsAsInts = false;
};

/**
 A Protobuf message that can be parsed from Protobuf bytes and serialized into
 bytes. This implementation is designed to offer the best performance possible
 in an entirely reflection based environment.
 */
class Message : public SimpleRefCountable {
public:
    Message();
    Message(const google::protobuf::Descriptor* descriptor, const Ref<RefCountable>& dataSource);
    ~Message() override;

    size_t encodedByteSize(bool includeEmptyFields = false);
    size_t getCachedEncodedByteSize() const;

    BytesView encode(bool includeEmptyFields = false);
    Byte* encode(bool includeEmptyFields, Byte* bufferStart, Byte* bufferEnd) const;

    bool decode(const Byte* data, size_t length, ExceptionTracker& exceptionTracker);

    bool decodeFromJSON(std::string_view json, ExceptionTracker& exceptionTracker);

    Ref<Message> clone() const;

    void clearAllFields();

    void appendField(FieldNumber fieldNumber, Field field);
    void clearField(FieldNumber fieldNumber);

    std::vector<FieldNumber> sortedFieldNumbers() const;

    std::optional<FieldNumber> getFieldNumberForName(std::string_view fieldName) const;

    Field* getFieldByName(std::string_view fieldName);
    Field* getField(FieldNumber fieldNumber);
    Field& getOrCreateField(FieldNumber fieldNumber);
    const Field* getFieldByName(std::string_view fieldName) const;
    const Field* getField(FieldNumber fieldNumber) const;

    std::string_view getFieldAsString(FieldNumber fieldNumber) const;

    RepeatedFieldIterator getFieldIterator(FieldNumber fieldNumber);
    RepeatedFieldConstIterator getFieldIterator(FieldNumber fieldNumber) const;
    RepeatedFieldIterator getFieldIteratorForName(std::string_view fieldName);
    RepeatedFieldConstIterator getFieldIteratorForName(std::string_view fieldName) const;

    const Ref<RefCountable>& getDataSource() const;

    /**
     Postprocess will validate the parsed Message and transform Message fields from raw bytes into Message objects.
     If recursive is true, nested messages will also be postprocessed.
     */
    bool postprocess(bool recursive, IMessageFactory& messageFactory, ExceptionTracker& exceptionTracker);

    /**
     Postprocess will validate the parsed Message and transform Message fields from raw bytes into Message objects.
     If recursive is true, nested messages will also be postprocessed.
     */
    bool postprocess(bool recursive, ExceptionTracker& exceptionTracker);

    const google::protobuf::Descriptor* getDescriptor() const;

    std::string toJSON(const JSONPrintOptions& options, ExceptionTracker& exceptionTracker);

    static Ref<Message> parse(const BytesView& bytes,
                              const google::protobuf::Descriptor* descriptor,
                              ExceptionTracker& exceptionTracker);
    static Result<Ref<Message>> parse(const BytesView& bytes, const google::protobuf::Descriptor* descriptor);

    static Ref<Message> parseFromJSON(std::string_view json,
                                      const google::protobuf::Descriptor* descriptor,
                                      ExceptionTracker& exceptionTracker);

    static Result<Ref<Message>> parseFromJSON(std::string_view json, const google::protobuf::Descriptor* descriptor);

    static void setLogger(Valdi::ILogger* logger);

private:
    const google::protobuf::Descriptor* _descriptor = nullptr;
    Ref<RefCountable> _dataSource;
    FieldMap _fieldMap;
    size_t _cachedEncodedByteSize = 0;

    friend Message;

    bool populateFieldFlags();

    bool onDecodeError(
        std::string_view message, int fieldNumber, const Byte* data, size_t length, ExceptionTracker& exceptionTracker);

    bool postprocessForField(const google::protobuf::FieldDescriptor& fieldDescriptor,
                             Field& fieldValue,
                             bool recursive,
                             IMessageFactory& messageFactory,
                             ExceptionTracker& exceptionTracker);
};

class IMessageFactory {
public:
    virtual ~IMessageFactory() = default;

    virtual Ref<Message> newMessage(const google::protobuf::Descriptor* descriptor,
                                    const Ref<RefCountable>& dataSource) = 0;
};

} // namespace Valdi::Protobuf
