//
//  ImageCacheItem.cpp
//  valdi-desktop-apple
//
//  Created by Ramzy Jaber on 5/16/22.
//

#include "valdi/snap_drawing/ImageLoading/ImageCacheItem.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"
#include "valdi_core/cpp/Constants.hpp"

namespace snap::drawing {

snap::utils::time::Duration<std::chrono::steady_clock> ImageCacheItem::getCurrentTime() {
    return snap::utils::time::Duration<std::chrono::steady_clock>(std::chrono::steady_clock::now().time_since_epoch());
}

size_t ImageCacheItem::imageSize(const Valdi::Ref<Image>& image) {
    auto width = image->width();
    auto height = image->height();
    auto bytesPerPixel = image->getSkValue()->imageInfo().bytesPerPixel();
    return width * height * bytesPerPixel;
}

ImageCacheItem::ImageCacheItem(const String& url,
                               const Valdi::Ref<ImageCacheItem>& parent,
                               const Valdi::Ref<Image>& img)
    : _variants(0), _url(url), _parent(parent.get()), _image(img) {
    updateLastAccess();
    _previous = nullptr;
    _next = nullptr;
};

ImageCacheItem::~ImageCacheItem() {
    if (_parent != nullptr) {
        _parent->decrementVariants();
        _parent = nullptr;
    }
}

bool ImageCacheItem::isUnused() const {
    return _image->retainCount() == 1;
}

long ImageCacheItem::getImageSizeInBytes() const {
    return imageSize(_image);
}

const Valdi::Ref<Image>& ImageCacheItem::updateLastAccessAndGetImage() {
    updateLastAccess();
    return _image;
}

const String& ImageCacheItem::getUrl() const {
    return _url;
}

Valdi::Ref<ImageCacheItem> ImageCacheItem::getParent() const {
    return Valdi::strongSmallRef(_parent);
}

const String& ImageCacheItem::getOriginalUrl() const {
    if (isOriginal()) {
        return _url;
    } else {
        return _parent->_url;
    }
}

bool ImageCacheItem::isOriginal() const {
    return _parent == nullptr;
}

bool ImageCacheItem::hasVariants() const {
    return _variants != 0;
}

int ImageCacheItem::getVariantCount() const {
    return static_cast<int>(_variants);
}

int ImageCacheItem::getWidth() const {
    return _image->width();
}

int ImageCacheItem::getHeight() const {
    return _image->height();
}

Valdi::Ref<ImageCacheItem> ImageCacheItem::getResized(const String& url, int width, int height) {
    _variants++;
    auto strongRef = Valdi::strongSmallRef(this);
    return Valdi::makeShared<ImageCacheItem>(url, strongRef, _image->resized(width, height));
}

bool ImageCacheItem::isExpired(snap::utils::time::Duration<std::chrono::steady_clock> time) const {
    return time >= _lastAccessed;
}

void ImageCacheItem::updateLastAccess() {
    _lastAccessed = getCurrentTime();
}

void ImageCacheItem::decrementVariants() {
    _variants--;
}

Valdi::Ref<ImageCacheItem> ImageCacheItem::getPrevious() const {
    return Valdi::strongSmallRef(_previous);
}

void ImageCacheItem::insertAfter(const Valdi::Ref<ImageCacheItem>& imageCacheItem) {
    SC_ASSERT(imageCacheItem->_previous == nullptr && imageCacheItem->_next == nullptr);
    imageCacheItem->_next = _next;
    _next = imageCacheItem;

    imageCacheItem->_previous = this;
    if (imageCacheItem->_next != nullptr) {
        imageCacheItem->_next->_previous = imageCacheItem.get();
    }
}

Valdi::Ref<ImageCacheItem> ImageCacheItem::disconnectAndReturnPrevious() {
    auto retval = _previous;

    if (_next != nullptr) {
        _next->_previous = _previous;
    }

    if (_previous != nullptr) {
        _previous->_next = _next;
    }

    _previous = nullptr;
    _next = nullptr;

    return Valdi::strongSmallRef(retval);
}

} // namespace snap::drawing
