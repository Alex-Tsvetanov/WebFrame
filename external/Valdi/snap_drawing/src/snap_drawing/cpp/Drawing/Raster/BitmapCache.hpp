#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Interfaces/IBitmap.hpp"
#include "valdi_core/cpp/Interfaces/IBitmapFactory.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include <vector>

namespace Valdi {
class IBitmapFactory;
}

namespace snap::drawing {

/**
BitmapCache helps with creating bitmaps and re-using them when they are not already in-use.
 */
class BitmapCache {
public:
    BitmapCache();
    ~BitmapCache();

    Valdi::Result<Ref<Valdi::IBitmap>> allocateBitmap(const Ref<Valdi::IBitmapFactory>& bitmapFactory,
                                                      int width,
                                                      int height);

    void clearUnused();

private:
    class Entry {
    public:
        Entry(const Ref<Valdi::IBitmapFactory>& bitmapFactory,
              const Ref<Valdi::IBitmap>& bitmap,
              int width,
              int height);
        ~Entry();

        bool isCompatibleWith(const Ref<Valdi::IBitmapFactory>& bitmapFactory, int width, int height) const;

        bool isUsed() const;

        const Ref<Valdi::IBitmap>& getBitmap() const;

    private:
        Ref<Valdi::IBitmapFactory> _bitmapFactory;
        Ref<Valdi::IBitmap> _bitmap;
        int _width;
        int _height;
    };

    std::vector<Entry> _entries;
};

} // namespace snap::drawing