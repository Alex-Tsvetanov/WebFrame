//
//  ZStdUtils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 3/8/19.
//

#include "valdi/runtime/Resources/ZStdUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "zstd.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace Valdi {

bool ZStdUtils::isZstdFile(const Byte* input, size_t length) {
    if (length < 4) {
        return false;
    }
    uint32_t output;
    std::memcpy(&output, input, sizeof(uint32_t));
    return output == ZSTD_MAGICNUMBER;
}

Result<Ref<ByteBuffer>> ZStdUtils::decompress(const Byte* input, size_t len) {
    auto* dstream = ZSTD_createDStream();
    if (dstream == nullptr) {
        return Error("Could not create ZSTD stream");
    }

    auto bufferSize = ZSTD_DStreamOutSize();

    ByteBuffer buffer;
    buffer.resize(bufferSize);

    /* In more complex scenarios, a file may consist of multiple appended frames (ex : pzstd).
     *  The following example decompresses only the first frame.
     *  It is compatible with other provided streaming examples */
    auto initResult = ZSTD_initDStream(dstream);
    if (ZSTD_isError(initResult) != 0) {
        ZSTD_freeDStream(dstream);
        return Error(STRING_FORMAT("Could not initialize stream: {}", ZSTD_getErrorName(initResult)));
    }

    ZSTD_inBuffer inBuffer;
    inBuffer.src = input;
    inBuffer.size = len;
    inBuffer.pos = 0;

    ZSTD_outBuffer outBuffer;
    outBuffer.dst = buffer.data();
    outBuffer.pos = 0;
    outBuffer.size = bufferSize;

    auto output = makeShared<ByteBuffer>();
    output->reserve(len * 4);

    while (inBuffer.pos < len) {
        auto result = ZSTD_decompressStream(dstream, &outBuffer, &inBuffer);
        if (ZSTD_isError(result) != 0) {
            ZSTD_freeDStream(dstream);
            return Error(STRING_FORMAT("Could not decompress stream: {}", ZSTD_getErrorName(result)));
        }

        auto written = outBuffer.pos;

        output->append(buffer.begin(), buffer.begin() + written);
        outBuffer.pos = 0;
    }

    ZSTD_freeDStream(dstream);
    output->shrinkToFit();

    return output;
}
} // namespace Valdi
