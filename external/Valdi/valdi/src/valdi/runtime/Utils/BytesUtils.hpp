//
//  BytesUtils.hpp
//  valdi-ios
//
//  Created by saniul on 11/5/19.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include <string>

namespace Valdi {

class BytesUtils {
public:
    static Ref<ByteBuffer> sha256(const BytesView& data);
    static Ref<ByteBuffer> sha256(const Byte* data, size_t length);

    static std::string sha256String(const BytesView& data);
    static std::string sha256String(const Byte* data, size_t length);
};

} // namespace Valdi
