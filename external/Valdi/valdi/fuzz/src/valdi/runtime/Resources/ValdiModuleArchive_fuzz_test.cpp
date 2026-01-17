#include "valdi/runtime/Resources/ValdiModuleArchive.hpp"

#include <fuzzer/FuzzedDataProvider.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    Valdi::ValdiModuleArchive::decompress(data, size);
    return 0;
}
