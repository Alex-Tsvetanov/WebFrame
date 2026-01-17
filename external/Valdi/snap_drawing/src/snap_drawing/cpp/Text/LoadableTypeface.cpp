//
//  LoadableTypeface.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/2/20.
//

#include "snap_drawing/cpp/Text/LoadableTypeface.hpp"
#include "include/core/SkData.h"
#include "include/core/SkFontMgr.h"
#include "snap_drawing/cpp/Utils/BytesUtils.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"

namespace snap::drawing {

class BytesLoadableTypeface : public LoadableTypeface {
public:
    BytesLoadableTypeface(const String& fontName, const Valdi::BytesView& bytes)
        : LoadableTypeface(fontName), _bytes(bytes) {}
    ~BytesLoadableTypeface() override = default;

protected:
    Valdi::Result<Valdi::BytesView> loadFontData() override {
        return std::move(_bytes);
    }

private:
    Valdi::BytesView _bytes;
};

class FileLoadableTypeface : public LoadableTypeface {
public:
    FileLoadableTypeface(const String& fontName, const String& path) : LoadableTypeface(fontName), _path(path) {}
    ~FileLoadableTypeface() override = default;

protected:
    Valdi::Result<Valdi::BytesView> loadFontData() override {
        Valdi::Path path(_path.toStringView());
        return Valdi::DiskUtils::load(path);
    }

private:
    Valdi::StringBox _path;
};

LoadableTypeface::LoadableTypeface(const String& fontName) : _fontName(fontName) {}
LoadableTypeface::~LoadableTypeface() = default;

const Valdi::Result<Ref<Typeface>>& LoadableTypeface::get(SkFontMgr& fontMgr) {
    if (_loadedTypeface.empty()) {
        _loadedTypeface = loadTypeface(fontMgr);
    }
    return _loadedTypeface;
}

const String& LoadableTypeface::getName() const {
    return _fontName;
}

Valdi::Result<Ref<Typeface>> LoadableTypeface::loadTypeface(SkFontMgr& fontMgr) {
    auto fontDataResult = loadFontData();
    if (!fontDataResult) {
        return fontDataResult.error().rethrow("Could not load typeface data");
    }

    auto fontData = fontDataResult.moveValue();
    auto skData = skDataFromBytes(fontData, DataConversionModeCopyIfUnsafe);
    auto skTypeface = fontMgr.makeFromData(skData);

    if (skTypeface == nullptr) {
        return Valdi::Error("Could not load typeface from data");
    }

    return Valdi::makeShared<Typeface>(std::move(skTypeface), _fontName, true);
}

Ref<LoadableTypeface> LoadableTypeface::fromBytes(const String& fontName, const Valdi::BytesView& bytes) {
    return Valdi::makeShared<BytesLoadableTypeface>(fontName, bytes);
}

Ref<LoadableTypeface> LoadableTypeface::fromFile(const String& fontName, const Valdi::StringBox& filePath) {
    return Valdi::makeShared<FileLoadableTypeface>(fontName, filePath);
}

} // namespace snap::drawing
