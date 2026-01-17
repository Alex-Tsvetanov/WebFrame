//
//  ValdiPacket.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/8/19.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace Valdi {

class ValdiPacket {
public:
    /**
     The minimum size that a packet can be
     */
    static size_t minSize();

    /**
     Read the packet, returns a BytesView containing the data section
     */
    static Result<BytesView> read(const Byte* data, size_t size);

    /**
     Writes the data section as a Valdi packet into the given Bytes,
     returns how many bytes were written.
     */
    static size_t write(const Byte* data, size_t size, ByteBuffer& out);

    /**
     Error sent when the packet is not complete
     */
    static const Error& incompletePacketError();
};

class ValdiPacketStream {
public:
    ValdiPacketStream();
    ~ValdiPacketStream();

    void write(const BytesView& bytes);
    Result<BytesView> read();

private:
    ByteBuffer _buffer;
};

} // namespace Valdi
