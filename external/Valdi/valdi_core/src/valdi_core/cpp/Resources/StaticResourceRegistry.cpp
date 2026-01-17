#include "valdi_core/cpp/Resources/StaticResourceRegistry.hpp"

namespace Valdi {

StaticResourceRegistry::StaticResourceRegistry() {
    _resources.reserve(128);
}

StaticResourceRegistry::~StaticResourceRegistry() = default;

void StaticResourceRegistry::registerResource(const char* path,
                                              const uint8_t* bytes,
                                              size_t length,
                                              bool requiresDecompression) {
    _resources.emplace_back(path, bytes, length, requiresDecompression);
}

const std::vector<StaticResourceRegistry::Resource>& StaticResourceRegistry::getResources() const {
    return _resources;
}

StaticResourceRegistry& StaticResourceRegistry::sharedInstance() {
    static auto* kInstance = new StaticResourceRegistry();
    return *kInstance;
}

} // namespace Valdi