//
//  ValdiPacket.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/8/19.
//

#include "valdi_core/cpp/Resources/ValdiPacket.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Parser.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

struct ValdiPacketHeader {
    uint32_t magic;
    uint32_t totalDataLength;
};

constexpr uint32_t kValdiMagic = 0x0100C633;

/// Uses a simple protocol:
///  __________________________________________________________
/// |                                                          |
/// | 4 bytes MAGIC | 4 bytes DATA LENGTH (XX) | XX bytes DATA |
/// |__________________________________________________________|

size_t ValdiPacket::minSize() {
    return sizeof(ValdiPacketHeader);
}

Result<BytesView> ValdiPacket::read(const Byte* data, size_t size) {
    const auto* dataEnd = data + size;
    auto parser = Parser(data, dataEnd);

    auto moduleHeaderResult = parser.parseStruct<ValdiPacketHeader>();
    if (!moduleHeaderResult) {
        return ValdiPacket::incompletePacketError();
    }

    const auto& moduleHeader = moduleHeaderResult.value();
    if (moduleHeader->magic != kValdiMagic) {
        return Error(STRING_FORMAT("magic is incorrect, expected {} got {}", kValdiMagic, moduleHeader->magic));
    }

    auto expectedLen = static_cast<size_t>(moduleHeader->totalDataLength);
    const auto* expectedEndPtr = parser.getCurrent() + expectedLen;
    if (expectedEndPtr > dataEnd) {
        return ValdiPacket::incompletePacketError();
    }

    return BytesView(nullptr, parser.getCurrent(), expectedLen);
}

size_t ValdiPacket::write(const Byte* data, size_t size, ByteBuffer& out) {
    ValdiPacketHeader header;
    header.magic = kValdiMagic;
    header.totalDataLength = static_cast<uint32_t>(size);

    out.reserve(out.size() + sizeof(ValdiPacketHeader) + size);
    out.append(reinterpret_cast<const Byte*>(&header),
               reinterpret_cast<const Byte*>(&header) + sizeof(ValdiPacketHeader));
    out.append(data, data + size);

    return size + sizeof(ValdiPacketHeader);
}

const Error& ValdiPacket::incompletePacketError() {
    static auto kError = Error("Incomplete packet");
    return kError;
}

ValdiPacketStream::ValdiPacketStream() = default;
ValdiPacketStream::~ValdiPacketStream() = default;

void ValdiPacketStream::write(const BytesView& bytes) {
    _buffer.append(bytes.begin(), bytes.end());
}

Result<BytesView> ValdiPacketStream::read() {
    if (_buffer.size() < ValdiPacket::minSize()) {
        return ValdiPacket::incompletePacketError();
    }

    auto result = ValdiPacket::read(_buffer.data(), _buffer.size());
    if (!result) {
        return result;
    }
    auto data = result.moveValue();

    auto copy = makeShared<ByteBuffer>();
    copy->append(data.begin(), data.end());

    _buffer.shift(data.end() - _buffer.data());

    return copy->toBytesView();
}

} // namespace Valdi
