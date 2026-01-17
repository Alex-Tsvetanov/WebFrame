//
//  ImageQueue.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 6/16/22.
//

#include "snap_drawing/cpp/Utils/ImageQueue.hpp"
#include "snap_drawing/cpp/Utils/Bitmap.hpp"

namespace snap::drawing {

PooledBitmap::PooledBitmap(ImageQueue* queue, Ref<Valdi::IBitmap> sourceBitmap)
    : _queue(Valdi::weakRef(queue)), _sourceBitmap(std::move(sourceBitmap)) {}

PooledBitmap::~PooledBitmap() {
    doDispose();
}

void PooledBitmap::doDispose() {
    auto queue = _queue.lock();
    if (queue != nullptr && _sourceBitmap != nullptr) {
        queue->enqueueCachedBitmap(_sourceBitmap);
        _sourceBitmap = nullptr;
    }
}

void PooledBitmap::dispose() {
    doDispose();
}

Valdi::BitmapInfo PooledBitmap::getInfo() const {
    return _sourceBitmap->getInfo();
}

void* PooledBitmap::lockBytes() {
    return _sourceBitmap->lockBytes();
}

void PooledBitmap::unlockBytes() {
    _sourceBitmap->unlockBytes();
}

ImageQueue::ImageQueue(size_t maxQueueSize) : _maxQueueSize(maxQueueSize) {}

ImageQueue::~ImageQueue() = default;

void ImageQueue::enqueue(const Ref<Image>& image) {
    std::unique_lock<Valdi::Mutex> lock(_mutex);
    _queue.emplace_back(image);
    if (_maxQueueSize != 0) {
        clearQueueUpToSize(_maxQueueSize, lock);
    }
}

void ImageQueue::clear() {
    std::unique_lock<Valdi::Mutex> lock(_mutex);
    clearQueueUpToSize(0, lock);
}

std::optional<Ref<Image>> ImageQueue::dequeue() {
    std::lock_guard<Valdi::Mutex> lock(_mutex);
    if (_queue.empty()) {
        return std::nullopt;
    }

    auto image = std::move(_queue.front());
    _queue.pop_front();

    return {std::move(image)};
}

void ImageQueue::clearQueueUpToSize(size_t maxSize, std::unique_lock<Valdi::Mutex>& lock) {
    while (_queue.size() > maxSize) {
        auto image = std::move(_queue.front());
        _queue.pop_front();

        lock.unlock();
        image = nullptr;
        lock.lock();
    }
}

bool ImageQueue::isCacheCompatibleWithBitmapInfo(const Valdi::BitmapInfo& bitmapInfo) const {
    if (!_cacheBitmapInfo) {
        return false;
    }

    return _cacheBitmapInfo->width == bitmapInfo.width && _cacheBitmapInfo->height == bitmapInfo.height &&
           _cacheBitmapInfo->colorType == bitmapInfo.colorType;
}

Valdi::Result<Ref<Valdi::IBitmap>> ImageQueue::allocateBitmap(int width, int height, Valdi::ColorType colorType) {
    auto bitmapInfo = Valdi::BitmapInfo(width, height, colorType, Valdi::AlphaType::AlphaTypePremul, 0);

    {
        std::lock_guard<Valdi::Mutex> lock(_mutex);
        if (!isCacheCompatibleWithBitmapInfo(bitmapInfo)) {
            _cacheBitmapInfo = {bitmapInfo};
            _cachedBitmaps.clear();
        }

        if (!_cachedBitmaps.empty()) {
            auto cachedBitmap = std::move(_cachedBitmaps.back());
            _cachedBitmaps.pop_back();

            Ref<Valdi::IBitmap> pooledBitmap = Valdi::makeShared<PooledBitmap>(this, std::move(cachedBitmap));
            return pooledBitmap;
        }
    }

    // Cache is empty, allocating the bitmap

    auto bitmap = Bitmap::make(bitmapInfo);
    if (!bitmap) {
        return bitmap.moveError();
    }

    Ref<Valdi::IBitmap> pooledBitmap = Valdi::makeShared<PooledBitmap>(this, bitmap.moveValue());
    return pooledBitmap;
}

void ImageQueue::enqueueCachedBitmap(const Ref<Valdi::IBitmap>& sourceBitmap) {
    std::lock_guard<Valdi::Mutex> lock(_mutex);
    auto bitmapInfo = sourceBitmap->getInfo();
    if (!isCacheCompatibleWithBitmapInfo(bitmapInfo) || _cachedBitmaps.size() >= kImageQueueMaxPoolSize) {
        return;
    }
    _cachedBitmaps.emplace_back(sourceBitmap);
}

size_t ImageQueue::getQueueSize() const {
    std::lock_guard<Valdi::Mutex> lock(_mutex);
    return _queue.size();
}

size_t ImageQueue::getPoolSize() const {
    std::lock_guard<Valdi::Mutex> lock(_mutex);
    return _cachedBitmaps.size();
}

} // namespace snap::drawing
