//
//  ShaderCache.cpp
//  snap_drawing-macos
//
//  Created by Ramzy Jaber on 4/1/22.
//

#include "valdi/snap_drawing/Graphics/ShaderCache.hpp"

#include "snap_drawing/cpp/Utils/BytesUtils.hpp"
#include "valdi/runtime/Utils/BytesUtils.hpp"
#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

// TODO(rjaber): The cached shaders are only ever valid if SnapDrawing and Skia don't change. The moment there's any
//               change, we need to invalidate the cache. There isn't a suitable mechanism for this; this value is
//               currently used as a stand-in until we have a more reliable mechanism.
static const int kShaderCacheVersion = 1;

using namespace Valdi;

namespace snap::drawing {

static Result<Valdi::Path> keyToFileName(const BytesView& key) {
    if (key.data() == nullptr || key.empty()) {
        return Error("Invalid key data object");
    }

    auto strHash = BytesUtils::sha256String(key);
    return Valdi::Path(strHash);
}

static Result<Valdi::BytesView> skDataToByteView(const SkData& data) {
    if (data.data() == nullptr || data.isEmpty()) {
        return Error("Provided shader data is empty");
    }

    auto retval = Valdi::makeShared<Valdi::ByteBuffer>();
    auto start_ptr = static_cast<const Valdi::Byte*>(data.data());
    auto end_ptr = static_cast<const Valdi::Byte*>(data.data()) + data.size();

    retval->append(start_ptr, end_ptr);
    return retval->toBytesView();
}

Result<BytesView> ShaderCache::loadShaderFromDisk(const BytesView& key) const {
    auto keyFilePathResult = keyToFileName(key);
    if (keyFilePathResult.failure()) {
        VALDI_ERROR(_logger, "Failed to generate file name from key");
        return keyFilePathResult.error();
    }
    auto keyFilePath = keyFilePathResult.moveValue();

    if (!_diskCache->exists(keyFilePath)) {
        return Error("Key not in cache");
    }

    auto loadResult = _diskCache->load(keyFilePath);
    if (loadResult.failure()) {
        VALDI_ERROR(_logger, "Failed to load content from {} with error {}", keyFilePath, loadResult.error());
        return loadResult.error();
    }

    return loadResult.value();
}

void ShaderCache::storeShaderToDisk(const BytesView& key, const BytesView& data) const {
    auto keyFilePathResult = keyToFileName(key);
    if (keyFilePathResult.failure()) {
        VALDI_ERROR(_logger, "Failed to generate file name from key");
        return;
    }
    auto keyFilePath = keyFilePathResult.moveValue();
    if (_diskCache->exists(keyFilePath)) {
        VALDI_WARN(_logger, "Overwriting shader item for {}", keyFilePath);
    }

    auto storeResult = _diskCache->store(keyFilePath, data);
    if (storeResult.failure()) {
        VALDI_ERROR(_logger, "Failed to store content for {} with error {}", keyFilePath, storeResult.error());
    }
}

ShaderCache::ShaderCache(const Ref<IDiskCache>& diskCache, const Ref<DispatchQueue>& dispatchQueue, ILogger& logger)
    : _logger(logger),
      _queue(dispatchQueue),
      _diskCache(diskCache->scopedCache(Path(fmt::format("{}", kShaderCacheVersion)), false)) {
    _queue->async([diskCache, scopedDiskCache = _diskCache]() {
        auto rootPath = diskCache->getRootPath();
        auto currRootPath = scopedDiskCache->getRootPath();
        for (const auto& folder : diskCache->list(rootPath)) {
            if (folder != currRootPath) {
                diskCache->remove(folder);
            }
        }
    });
}

void ShaderCache::store(const SkData& key, const SkData& data) {
    auto dataResult = skDataToByteView(data);
    auto ownedKeyResult = skDataToByteView(key);

    if (!dataResult || !ownedKeyResult) {
        VALDI_WARN(_logger, "Failed to copy data for shader store");
        return;
    }
    auto dataByteView = dataResult.moveValue();
    auto keyByteView = ownedKeyResult.moveValue();

    auto weakThis = weakRef(this);
    _queue->async([weakThis, dataByteView, keyByteView]() {
        auto strongThis = weakThis.lock();
        if (strongThis) {
            strongThis->storeShaderToDisk(keyByteView, dataByteView);
        }
    });
}

sk_sp<SkData> ShaderCache::load(const SkData& key) {
    auto keyByteViewUnowned = Valdi::BytesView(nullptr, static_cast<const Byte*>(key.data()), key.size());
    auto loadResult = loadShaderFromDisk(keyByteViewUnowned);
    if (loadResult.failure()) {
        return nullptr;
    } else {
        auto result = loadResult.moveValue();
        // TODO(rjaber): Data copy is temporary; Skia expects the returned SkData pointer to be 4 bytes aligned. By
        //               doing the copy, we rely on SkData being correctly aligned on creation.
        return snap::drawing::skDataFromBytes(result, snap::drawing::DataConversionModeAlwaysCopy);
    }
}

} // namespace snap::drawing
