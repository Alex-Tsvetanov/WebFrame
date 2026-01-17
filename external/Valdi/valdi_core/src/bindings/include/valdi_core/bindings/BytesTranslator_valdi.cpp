#include "valdi_core/bindings/BytesTranslator_valdi.hpp"
#include "valdi_core/cpp/Schema/ValueSchema.hpp"

namespace Valdi {

const ValueSchema& BytesTranslator::schema() {
    static auto schema = ValueSchema::valueTypedArray();
    return schema;
}

} // namespace Valdi
