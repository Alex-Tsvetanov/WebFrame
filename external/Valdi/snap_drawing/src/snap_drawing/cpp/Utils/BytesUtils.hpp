//
//  BytesUtils.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#pragma once

#include "include/core/SkData.h"
#include "valdi_core/cpp/Utils/Bytes.hpp"

namespace snap::drawing {

enum DataConversionMode {
    DataConversionModeCopyIfUnsafe,
    DataConversionModeAlwaysCopy,
    DataConversionModeNeverCopy,
};

Valdi::BytesView bytesFromSkData(const sk_sp<SkData>& data);
sk_sp<SkData> skDataFromBytes(const Valdi::BytesView& bytes, DataConversionMode conversionMode);

} // namespace snap::drawing
