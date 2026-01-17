
#include "valdi_core/cpp/Utils/DjinniUtils.hpp"

#include "djinni/cpp/DataRef.hpp"
#include <utility>

namespace Valdi {

VALDI_CLASS_IMPL(VectorOfValues)
VALDI_CLASS_IMPL(ValdiOutcome)

class DataRefSimpleRef : public SimpleRefCountable {
public:
    explicit DataRefSimpleRef(const djinni::DataRef& dataRef) : _dataRef(dataRef) {}

    const djinni::DataRef& getDataRef() const {
        return _dataRef;
    }

private:
    djinni::DataRef _dataRef;
};

class DataRefValdi : public djinni::DataRef::Impl {
public:
    explicit DataRefValdi(const Valdi::BytesView& bytesView) : _bytesView(bytesView) {}
    ~DataRefValdi() override = default;

    const uint8_t* buf() const override {
        return _bytesView.data();
    }

    size_t len() const override {
        return _bytesView.size();
    }

    uint8_t* mutableBuf() override {
        return const_cast<uint8_t*>(_bytesView.data());
    }

private:
    BytesView _bytesView;
};

Valdi::BytesView bytesViewFromDataRef(const djinni::DataRef& dataRef) {
    auto dataRefSimpleRef = Valdi::makeShared<DataRefSimpleRef>(dataRef);
    return Valdi::BytesView(dataRefSimpleRef, dataRef.buf(), dataRef.len());
}

djinni::DataRef dataRefFromBytesView(const Valdi::BytesView& bytesView) {
    auto existingDataRef = castOrNull<DataRefSimpleRef>(bytesView.getSource());
    if (existingDataRef != nullptr && existingDataRef->getDataRef().buf() == bytesView.data() &&
        existingDataRef->getDataRef().len() == bytesView.size()) {
        return existingDataRef->getDataRef();
    }

    return djinni::DataRef(std::make_shared<DataRefValdi>(bytesView));
}
} // namespace Valdi
