#pragma once

#include <cstdint>
#include <vector>

namespace Valdi {

/**
 * The StaticResourceRegistry holds the resources that are embedded into the binary.
 * This helps with making single file binaries that are fully self contained.
 */
class StaticResourceRegistry {
public:
    struct Resource {
        const char* path;
        const uint8_t* bytes;
        size_t length;
        bool requiresDecompression;

        constexpr Resource(const char* path, const uint8_t* bytes, size_t length, bool requiresDecompression)
            : path(path), bytes(bytes), length(length), requiresDecompression(requiresDecompression) {}
    };

    StaticResourceRegistry();
    ~StaticResourceRegistry();

    void registerResource(const char* path, const uint8_t* bytes, size_t length, bool requiresDecompression);

    const std::vector<Resource>& getResources() const;

    static StaticResourceRegistry& sharedInstance();

private:
    std::vector<Resource> _resources;
};

#define VALDI_REGISTER_STATIC_RESOURCE(__path__, __bytes__, __length__, __requires_decompression__)                    \
    Valdi::StaticResourceRegistry::sharedInstance().registerResource(                                                  \
        __path__, __bytes__, __length__, __requires_decompression__);

} // namespace Valdi