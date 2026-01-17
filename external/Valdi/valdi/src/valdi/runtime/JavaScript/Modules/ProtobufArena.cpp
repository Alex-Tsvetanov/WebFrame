//
//  ProtobufArena.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/21/23.
//

#include "valdi/runtime/JavaScript/Modules/ProtobufArena.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"

namespace Valdi {

JSProtobufMessage::JSProtobufMessage(size_t messageIndex,
                                     const google::protobuf::Descriptor* descriptor,
                                     const Ref<RefCountable>& dataSource)
    : Protobuf::Message(descriptor, dataSource), _messageIndex(messageIndex) {}

JSProtobufMessage::~JSProtobufMessage() = default;

size_t JSProtobufMessage::getMessageIndex() const {
    return _messageIndex;
}

ProtobufArena::ProtobufArena(bool eagerDecoding, bool includeAllFieldsDuringEncoding)
    : _eagerDecoding(eagerDecoding), _includeAllFieldsDuringEncoding(includeAllFieldsDuringEncoding) {}
ProtobufArena::~ProtobufArena() = default;

std::unique_lock<std::recursive_mutex> ProtobufArena::lock() const {
    return std::unique_lock<std::recursive_mutex>(_mutex);
}

size_t ProtobufArena::createMessage(const Ref<ProtobufMessageFactory>& messageFactory,
                                    size_t descriptorIndex,
                                    ExceptionTracker& exceptionTracker) {
    const auto* descriptor = messageFactory->getDescriptorAtIndex(descriptorIndex, exceptionTracker);
    if (descriptor == nullptr) {
        return 0;
    }

    retainMessageFactory(messageFactory);
    return createMessageForDescriptor(descriptor, nullptr)->getMessageIndex();
}

size_t ProtobufArena::decodeMessage(const Ref<ProtobufMessageFactory>& messageFactory,
                                    size_t descriptorIndex,
                                    const BytesView& bytes,
                                    bool isFromAsyncCall,
                                    ExceptionTracker& exceptionTracker) {
    const auto* descriptor = messageFactory->getDescriptorAtIndex(descriptorIndex, exceptionTracker);
    if (descriptor == nullptr) {
        return 0;
    }

    VALDI_TRACE_META("Protobuf.decodeMessage", descriptor->name());

    auto message = createMessageForDescriptor(descriptor, bytes.getSource());

    if (!message->decode(bytes.data(), bytes.size(), exceptionTracker)) {
        return 0;
    }

    return postProcessDecodedMessage(messageFactory, message, isFromAsyncCall, exceptionTracker);
}

size_t ProtobufArena::decodeMessageFromJSON(const Ref<ProtobufMessageFactory>& messageFactory,
                                            size_t descriptorIndex,
                                            std::string_view json,
                                            bool isFromAsyncCall,
                                            ExceptionTracker& exceptionTracker) {
    const auto* descriptor = messageFactory->getDescriptorAtIndex(descriptorIndex, exceptionTracker);
    if (descriptor == nullptr) {
        return 0;
    }

    VALDI_TRACE_META("Protobuf.decodeMessageFromJSON", descriptor->name());

    auto message = createMessageForDescriptor(descriptor, nullptr);

    if (!message->decodeFromJSON(json, exceptionTracker)) {
        return 0;
    }

    return postProcessDecodedMessage(messageFactory, message, isFromAsyncCall, exceptionTracker);
}

size_t ProtobufArena::postProcessDecodedMessage(const Ref<ProtobufMessageFactory>& messageFactory,
                                                const Ref<JSProtobufMessage>& message,
                                                bool isFromAsyncCall,
                                                ExceptionTracker& exceptionTracker) {
    retainMessageFactory(messageFactory);

    if (isFromAsyncCall || _eagerDecoding) {
        if (!message->postprocess(true, *this, exceptionTracker)) {
            return 0;
        }
    }

    return message->getMessageIndex();
}

Ref<Protobuf::Message> ProtobufArena::newMessage(const google::protobuf::Descriptor* descriptor,
                                                 const Ref<RefCountable>& dataSource) {
    return createMessageForDescriptor(descriptor, dataSource);
}

JSProtobufMessage* ProtobufArena::getMessage(size_t messageIndex, ExceptionTracker& exceptionTracker) const {
    if (messageIndex >= _messages.size()) {
        exceptionTracker.onError(Error("Invalid message"));
        return nullptr;
    }

    return _messages[messageIndex].get();
}

BytesView ProtobufArena::encodeMessage(size_t messageIndex, ExceptionTracker& exceptionTracker) const {
    auto* message = getMessage(messageIndex, exceptionTracker);
    if (message == nullptr) {
        return BytesView();
    }

    VALDI_TRACE_META("Protobuf.encodeMessage", message->getDescriptor()->name());

    return message->encode(_includeAllFieldsDuringEncoding);
}

size_t ProtobufArena::fieldToMessageIndex(JSProtobufMessage& message,
                                          Protobuf::Field& field,
                                          const google::protobuf::Descriptor* descriptor,
                                          ExceptionTracker& exceptionTracker) {
    auto raw = field.getRaw();
    if (raw.data != nullptr) {
        // Parse the bytes of the message into a concrete message instance
        auto outputMessage = createMessageForDescriptor(descriptor, message.getDataSource());

        if (!outputMessage->decode(raw.data, static_cast<size_t>(raw.length), exceptionTracker)) {
            return 0;
        }

        field.setMessage(outputMessage.get());
        return outputMessage->getMessageIndex();
    }

    auto* nestedMessage = dynamic_cast<JSProtobufMessage*>(field.getMessage());
    if (nestedMessage == nullptr) {
        exceptionTracker.onError(Error("Nested message is not a message"));
        return 0;
    }

    return nestedMessage->getMessageIndex();
}

size_t ProtobufArena::copyMessage(const ProtobufArena& fromArena,
                                  size_t messageIndex,
                                  ExceptionTracker& exceptionTracker) {
    auto* message = fromArena.getMessage(messageIndex, exceptionTracker);
    if (message == nullptr) {
        return 0;
    }

    // We just encode/decode to simplify the implementation
    // In practice there should be almost never any cases where messages are transferred between arenas.

    VALDI_TRACE_META("Protobuf.copyMessage", message->getDescriptor()->name());

    auto encoded = message->encode(_includeAllFieldsDuringEncoding);

    // Also retain the message factories
    for (const auto& messageFactory : fromArena._retainedMessageFactories) {
        retainMessageFactory(messageFactory);
    }

    auto outputMessage = createMessageForDescriptor(message->getDescriptor(), encoded.getSource());

    if (!outputMessage->decode(encoded.data(), encoded.size(), exceptionTracker)) {
        return 0;
    }

    return outputMessage->getMessageIndex();
}

std::string ProtobufArena::messageToJSON(size_t messageIndex,
                                         const Protobuf::JSONPrintOptions& printOptions,
                                         ExceptionTracker& exceptionTracker) const {
    auto message = getMessage(messageIndex, exceptionTracker);
    if (!exceptionTracker) {
        return "";
    }

    return message->toJSON(printOptions, exceptionTracker);
}

void ProtobufArena::retainMessageFactory(const Ref<ProtobufMessageFactory>& messageFactory) {
    for (const auto& existingMessageFactory : _retainedMessageFactories) {
        if (existingMessageFactory == messageFactory) {
            return;
        }
    }

    _retainedMessageFactories.emplace_back(messageFactory);
}

Ref<JSProtobufMessage> ProtobufArena::createMessageForDescriptor(const google::protobuf::Descriptor* descriptor,
                                                                 const Ref<RefCountable>& dataSource) {
    auto messageIndex = _messages.size();
    auto message = makeShared<JSProtobufMessage>(messageIndex, descriptor, dataSource);
    _messages.emplace_back(message);
    return message;
}

VALDI_CLASS_IMPL(ProtobufArena)

} // namespace Valdi
