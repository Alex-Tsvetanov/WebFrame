#include "BitmapCache.hpp"
#include "valdi_core/cpp/Interfaces/IBitmapFactory.hpp"

namespace snap::drawing {

BitmapCache::Entry::Entry(const Ref<Valdi::IBitmapFactory>& bitmapFactory,
                          const Ref<Valdi::IBitmap>& bitmap,
                          int width,
                          int height)
    : _bitmapFactory(bitmapFactory), _bitmap(bitmap), _width(width), _height(height) {}

BitmapCache::Entry::~Entry() = default;

bool BitmapCache::Entry::isCompatibleWith(const Ref<Valdi::IBitmapFactory>& bitmapFactory,
                                          int width,
                                          int height) const {
    return _bitmapFactory == bitmapFactory && _width == width && _height == height;
}

bool BitmapCache::Entry::isUsed() const {
    return _bitmap.use_count() > 1;
}

const Ref<Valdi::IBitmap>& BitmapCache::Entry::getBitmap() const {
    return _bitmap;
}

BitmapCache::BitmapCache() = default;
BitmapCache::~BitmapCache() = default;

Valdi::Result<Ref<Valdi::IBitmap>> BitmapCache::allocateBitmap(const Ref<Valdi::IBitmapFactory>& bitmapFactory,
                                                               int width,
                                                               int height) {
    for (const auto& entry : _entries) {
        if (!entry.isUsed() && entry.isCompatibleWith(bitmapFactory, width, height)) {
            return entry.getBitmap();
        }
    }

    auto bitmap = bitmapFactory->createBitmap(width, height);
    if (!bitmap) {
        return bitmap;
    }

    _entries.emplace_back(bitmapFactory, bitmap.value(), width, height);

    return bitmap;
}

void BitmapCache::clearUnused() {
    _entries.erase(std::remove_if(_entries.begin(), _entries.end(), [](const auto& entry) { return !entry.isUsed(); }),
                   _entries.end());
}
} // namespace snap::drawing