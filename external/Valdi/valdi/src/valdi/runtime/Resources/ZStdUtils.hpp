//
//  ZStdUtils.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 3/8/19.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include <vector>

namespace Valdi {

class ZStdUtils {
public:
    [[nodiscard]] static Result<Ref<ByteBuffer>> decompress(const Byte* input, size_t len);
    static bool isZstdFile(const Byte* input, size_t length);
};

} // namespace Valdi
