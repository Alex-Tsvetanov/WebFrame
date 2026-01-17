//
//  ImageQueue.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 6/16/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"

#include <deque>
#include <optional>

namespace snap::drawing {

class ImageQueue;

class PooledBitmap : public Valdi::IBitmap {
public:
    PooledBitmap(ImageQueue* queue, Ref<Valdi::IBitmap> sourceBitmap);
    ~PooledBitmap() override;

    void dispose() override;
    Valdi::BitmapInfo getInfo() const override;
    void* lockBytes() override;
    void unlockBytes() override;

private:
    Valdi::Weak<ImageQueue> _queue;
    Ref<Valdi::IBitmap> _sourceBitmap;

    void doDispose();
};

constexpr size_t kImageQueueMaxPoolSize = 3;

class ImageQueue : public Valdi::SharedPtrRefCountable {
public:
    explicit ImageQueue(size_t maxQueueSize);
    ~ImageQueue() override;

    void enqueue(const Ref<Image>& image);
    void clear();

    std::optional<Ref<Image>> dequeue();

    Valdi::Result<Ref<Valdi::IBitmap>> allocateBitmap(int width, int height, Valdi::ColorType colorType);

    size_t getQueueSize() const;
    size_t getPoolSize() const;

private:
    mutable Valdi::Mutex _mutex;
    std::deque<Ref<Image>> _queue;
    std::vector<Ref<Valdi::IBitmap>> _cachedBitmaps;
    std::optional<Valdi::BitmapInfo> _cacheBitmapInfo;
    size_t _maxQueueSize;

    friend PooledBitmap;

    void enqueueCachedBitmap(const Ref<Valdi::IBitmap>& sourceBitmap);
    void clearQueueUpToSize(size_t maxSize, std::unique_lock<Valdi::Mutex>& lock);

    bool isCacheCompatibleWithBitmapInfo(const Valdi::BitmapInfo& bitmapInfo) const;
};

} // namespace snap::drawing
