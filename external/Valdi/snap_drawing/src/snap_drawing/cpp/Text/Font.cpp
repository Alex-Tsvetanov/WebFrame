//
//  Font.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Text/Font.hpp"
#include "snap_drawing/cpp/Text/Typeface.hpp"

#include "include/core/SkFontMetrics.h"
#include <fmt/format.h>

namespace snap::drawing {

Font::Font(
    Ref<FontResolver>&& fontResolver, Ref<Typeface>&& typeface, Scalar size, double scale, bool respectDynamicType)
    : _fontResolver(std::move(fontResolver)),
      _typeface(std::move(typeface)),
      _font(_typeface->getSkValue(), size * scale),
      _size(size),
      _scale(scale),
      _respectDynamicType(respectDynamicType) {
    _font.setEdging(SkFont::Edging::kAntiAlias);
}

Font::~Font() = default;

FontId Font::getFontId() const {
    auto size = _font.getSize();

    auto sizeId = *reinterpret_cast<uint32_t*>(&size);
    auto typefaceId = _typeface->getId();

    return (static_cast<FontId>(sizeId) << 32) | static_cast<FontId>(typefaceId);
}

const SkFont& Font::getSkValue() const {
    return _font;
}

const FontStyle& Font::style() const {
    return _typeface->fontStyle();
}

Scalar Font::size() const {
    return _size;
}

Scalar Font::resolvedSize() const {
    return _font.getSize();
}

Ref<Font> Font::withScale(double scale) {
    if (_scale == scale) {
        return Valdi::strongSmallRef(this);
    }

    return Valdi::makeShared<Font>(
        Ref<FontResolver>(_fontResolver), Ref<Typeface>(_typeface), _size, scale, _respectDynamicType);
}

Valdi::Result<Ref<Font>> Font::withStyle(FontStyle style) {
    return withStyleAndSize(style, _size);
}

Valdi::Result<Ref<Font>> Font::withSize(Scalar size) {
    return withStyleAndSize(style(), size);
}

Valdi::Result<Ref<Font>> Font::withStyleAndSize(FontStyle style, Scalar size) {
    if (_typeface->fontStyle() == style && _size == size) {
        return Valdi::strongSmallRef(this);
    }

    return _fontResolver->getFont(_typeface, style, size, _scale, _respectDynamicType);
}

double Font::scale() const {
    return _scale;
}

const Ref<Typeface>& Font::typeface() const {
    return _typeface;
}

bool Font::respectDynamicType() const {
    return _respectDynamicType;
}

std::string Font::getDescription() const {
    return fmt::format("{} {} x{}", _typeface->familyName().toStringView(), _size, _scale);
}

const FontMetrics& Font::metrics() {
    if (!_loadedMetrics) {
        _loadedMetrics = true;
        _metrics = _typeface->getFontMetrics(_size * _scale);
    }

    return _metrics;
}

const HBFont& Font::getHBFont() {
    if (!_hasHBFont) {
        _hasHBFont = true;
        _hbFont = Harfbuzz::createSubFont(_typeface->getHBFont(), &_font);
    }

    return _hbFont;
}

VALDI_CLASS_IMPL(Font)

} // namespace snap::drawing
