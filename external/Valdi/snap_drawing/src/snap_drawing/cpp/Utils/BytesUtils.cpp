//
//  BytesUtils.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#include "snap_drawing/cpp/Utils/BytesUtils.hpp"

namespace snap::drawing {

class SkiaDataHolder : public Valdi::SimpleRefCountable {
public:
    explicit SkiaDataHolder(const sk_sp<SkData>& data) : _data(data) {}

    ~SkiaDataHolder() override = default;

    const sk_sp<SkData>& getData() const {
        return _data;
    }

private:
    sk_sp<SkData> _data;
};

Valdi::BytesView bytesFromSkData(const sk_sp<SkData>& data) {
    if (data == nullptr) {
        return Valdi::BytesView();
    }

    return Valdi::BytesView(
        Valdi::makeShared<SkiaDataHolder>(data), reinterpret_cast<const Valdi::Byte*>(data->data()), data->size());
}

static void releaseData(const void* /*ptr*/, void* context) {
    Valdi::unsafeBridgeRelease(context);
}

sk_sp<SkData> skDataFromBytes(const Valdi::BytesView& bytes, DataConversionMode conversionMode) {
    auto dataHolder = Valdi::castOrNull<SkiaDataHolder>(bytes.getSource());

    switch (conversionMode) {
        case DataConversionModeCopyIfUnsafe: {
            if (dataHolder != nullptr) {
                return dataHolder->getData();
            }

            if (bytes.empty()) {
                return SkData::MakeEmpty();
            }

            if (bytes.getSource() == nullptr) {
                return SkData::MakeWithCopy(bytes.data(), bytes.size());
            } else {
                auto* source = Valdi::unsafeBridgeRetain(bytes.getSource().get());
                return SkData::MakeWithProc(bytes.data(), bytes.size(), &releaseData, source);
            }
        }
        case DataConversionModeAlwaysCopy: {
            return SkData::MakeWithCopy(bytes.data(), bytes.size());
        }
        case DataConversionModeNeverCopy: {
            if (dataHolder != nullptr) {
                return dataHolder->getData();
            }

            if (bytes.getSource() == nullptr) {
                return SkData::MakeWithoutCopy(bytes.data(), bytes.size());
            } else {
                auto* source = Valdi::unsafeBridgeRetain(bytes.getSource().get());
                return SkData::MakeWithProc(bytes.data(), bytes.size(), &releaseData, source);
            }
        };
    }

    return nullptr;
}

} // namespace snap::drawing
