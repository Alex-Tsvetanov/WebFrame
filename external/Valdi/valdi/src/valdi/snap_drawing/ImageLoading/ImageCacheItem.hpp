//
//  ImageCacheItem.hpp
//  valdi-desktop-apple
//
//  Created by Ramzy Jaber on 5/16/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "utils/time/Duration.hpp"

namespace snap::drawing {

class Image;

class ImageCacheItem : public Valdi::SharedPtrRefCountable {
public:
    ImageCacheItem(const String& url, const Ref<ImageCacheItem>& parent, const Ref<Image>& img);
    ~ImageCacheItem() override;

    bool isUnused() const;

    long getImageSizeInBytes() const;

    const Ref<Image>& updateLastAccessAndGetImage();

    const String& getUrl() const;
    const String& getOriginalUrl() const;

    Ref<ImageCacheItem> getParent() const;

    bool isOriginal() const;
    bool hasVariants() const;
    int getVariantCount() const;

    int getWidth() const;

    int getHeight() const;

    Ref<ImageCacheItem> getResized(const String& url, int width, int height);

    bool isExpired(snap::utils::time::Duration<std::chrono::steady_clock> time) const;

    Ref<ImageCacheItem> getPrevious() const;
    void insertAfter(const Ref<ImageCacheItem>& imageCacheItem);
    Ref<ImageCacheItem> disconnectAndReturnPrevious();

    static snap::utils::time::Duration<std::chrono::steady_clock> getCurrentTime();
    static size_t imageSize(const Ref<Image>& image);

private:
    void updateLastAccess();
    void decrementVariants();

    uint64_t _variants;
    String _url;
    ImageCacheItem* _parent;
    Ref<Image> _image;
    snap::utils::time::Duration<std::chrono::steady_clock> _lastAccessed;

    ImageCacheItem* _previous;
    Ref<ImageCacheItem> _next;
};

} // namespace snap::drawing
