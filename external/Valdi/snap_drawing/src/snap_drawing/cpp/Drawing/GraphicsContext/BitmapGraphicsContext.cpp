//
//  BitmapGraphicsContext.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#include "snap_drawing/cpp/Drawing/GraphicsContext/BitmapGraphicsContext.hpp"
#include "include/core/SkSurface.h"
#include "snap_drawing/cpp/Utils/BitmapUtils.hpp"

namespace snap::drawing {

class BitmapDrawableSurface : public DrawableSurface {
public:
    BitmapDrawableSurface(const Valdi::Ref<Valdi::IBitmap>& bitmap,
                          const Valdi::BitmapInfo& bitmapInfo,
                          void* bitmapBytes)
        : _bitmap(bitmap),
          _bitmapInfo(bitmapInfo),
          _imageInfo(toSkiaImageInfo(_bitmapInfo)),
          _bitmapBytes(bitmapBytes) {}

    ~BitmapDrawableSurface() override {
        releaseSurface();
    }

    GraphicsContext* getGraphicsContext() override {
        return nullptr;
    }

    Valdi::Result<DrawableSurfaceCanvas> prepareCanvas() override {
        if (_surface == nullptr) {
            if (_bitmap != nullptr) {
                auto* bytes = _bitmap->lockBytes();

                if (bytes == nullptr) {
                    return Valdi::Error("Failed to lock bytes from Bitmap");
                }

                _surface = SkSurfaces::WrapPixels(_imageInfo, bytes, _bitmapInfo.rowBytes);

                if (_surface == nullptr) {
                    _bitmap->unlockBytes();
                    return Valdi::Error("Failed to create Surface");
                }
            } else if (_bitmapBytes != nullptr) {
                _surface = SkSurfaces::WrapPixels(_imageInfo, _bitmapBytes, _bitmapInfo.rowBytes);

                if (_surface == nullptr) {
                    return Valdi::Error("Failed to create Surface");
                }
            } else {
                _surface = SkSurfaces::Raster(_imageInfo);

                if (_surface == nullptr) {
                    return Valdi::Error("Failed to create Surface");
                }
            }
        }

        return DrawableSurfaceCanvas(_surface.get(), _bitmapInfo.width, _bitmapInfo.height);
    }

    void flush() override {
        releaseSurface();
    }

    void releaseSurface() {
        if (_surface != nullptr) {
            _surface = nullptr;

            if (_bitmap != nullptr) {
                _bitmap->unlockBytes();
            }
        }
    }

private:
    Valdi::Ref<Valdi::IBitmap> _bitmap;
    Valdi::BitmapInfo _bitmapInfo;
    SkImageInfo _imageInfo;
    sk_sp<SkSurface> _surface;
    void* _bitmapBytes = nullptr;
};

BitmapGraphicsContext::BitmapGraphicsContext() = default;

void BitmapGraphicsContext::setResourceCacheLimit(size_t /*resourceCacheLimitBytes*/) {}

void BitmapGraphicsContext::performCleanup(bool /*shouldPurgeScratchResources*/,
                                           std::chrono::seconds /*secondsNotUsed*/) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Ref<DrawableSurface> BitmapGraphicsContext::createBitmapSurface(const Valdi::Ref<Valdi::IBitmap>& bitmap) {
    return Valdi::makeShared<BitmapDrawableSurface>(bitmap, bitmap->getInfo(), nullptr);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Ref<DrawableSurface> BitmapGraphicsContext::createBitmapSurface(const Valdi::BitmapInfo& bitmapInfo) {
    return Valdi::makeShared<BitmapDrawableSurface>(nullptr, bitmapInfo, nullptr);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Ref<DrawableSurface> BitmapGraphicsContext::createBitmapSurface(const Valdi::BitmapInfo& bitmapInfo,
                                                                void* bitmapBytes) {
    return Valdi::makeShared<BitmapDrawableSurface>(nullptr, bitmapInfo, bitmapBytes);
}

Valdi::Result<Valdi::BytesView> BitmapGraphicsContext::bitmapToPNG(const Valdi::Ref<Valdi::IBitmap>& bitmap) {
    BitmapGraphicsContext bitmapContext;
    auto surface = bitmapContext.createBitmapSurface(bitmap);

    auto canvasResult = surface->prepareCanvas();
    if (!canvasResult) {
        return canvasResult.moveError();
    }

    auto image = canvasResult.value().snapshot();
    if (!image) {
        return image.moveError();
    }

    return image.value()->toPNG();
}

} // namespace snap::drawing
