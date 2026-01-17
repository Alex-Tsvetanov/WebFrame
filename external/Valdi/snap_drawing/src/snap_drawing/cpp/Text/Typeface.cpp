//
//  Typeface.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 1/28/22.
//

#include "snap_drawing/cpp/Text/Typeface.hpp"
#include "snap_drawing/cpp/Text/FontStyle.hpp"

#include "valdi_core/cpp/Text/CharacterSet.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"

#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkStream.h"

namespace snap::drawing {

constexpr Scalar kDefaultLineHeightToUnderlineThicknessRatio = 0.06;
constexpr Scalar kDefaultDescentUnderlineToPositionRatio = 0.2;

constexpr Scalar kDefaultLineHeightToStrikethroughThicknessRatio = 0.06;
constexpr Scalar kDefaultAscentToStrikethroughPositionRatio = 0.3;

Typeface::Typeface(sk_sp<SkTypeface>&& typeface, const String& familyName, bool isCustom)
    : _typeface(std::move(typeface)), _familyName(familyName), _fontStyle(_typeface->fontStyle()), _isCustom(isCustom) {
    _hbFace = Harfbuzz::createFace(_typeface.get());
    if (_hbFace.get() != nullptr) {
        _hbFont = Harfbuzz::createMainFont(*_typeface, _hbFace);
    }

    _characterSet = _hbFace.getCharacters();
    // This seems arbitrary, but I'm not sure if there is a better way
    _isEmoji = supportsCharacter(0x270C);
}

Typeface::~Typeface() = default;

uint32_t Typeface::getId() const {
    return _typeface->uniqueID();
}

const sk_sp<SkTypeface>& Typeface::getSkValue() const {
    return _typeface;
}

const String& Typeface::familyName() const {
    return _familyName;
}

const FontStyle& Typeface::fontStyle() const {
    return _fontStyle;
}

bool Typeface::isCustom() const {
    return _isCustom;
}

bool Typeface::supportsCharacter(Character character) {
    return _characterSet->contains(character);
}

const HBFace& Typeface::getHBFace() const {
    return _hbFace;
}

const HBFont& Typeface::getHBFont() const {
    return _hbFont;
}

bool Typeface::isEmoji() const {
    return _isEmoji;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool Typeface::hasSpaceInLigaturesOrKerning() const {
    // TODO(simon): Implement
    return false;
}

Valdi::BytesView Typeface::makeFontData() {
    auto skStreamAsset = _typeface->openStream(nullptr);
    if (!skStreamAsset) {
        return Valdi::BytesView();
    }

    auto streamSize = skStreamAsset->getLength();
    auto buffer = Valdi::makeShared<Valdi::ByteBuffer>();
    buffer->reserve(streamSize);

    auto realSize = skStreamAsset->read(buffer->data(), streamSize);
    buffer->resize(realSize);
    return buffer->toBytesView();
}

FontMetrics Typeface::getFontMetrics(double fontSize) {
    std::lock_guard<Valdi::Mutex> lock(_mutex);

    const auto& it = _fontMetricsBySize.find(fontSize);
    if (it != _fontMetricsBySize.end()) {
        return it->second;
    }

    SkFont font(_typeface, fontSize);
    font.setEdging(SkFont::Edging::kAntiAlias);

    SkFontMetrics metrics;

    FontMetrics out;
    out.lineSpacing = font.getMetrics(&metrics);
    out.descent = metrics.fDescent;
    out.ascent = metrics.fAscent;

    auto lineHeight = metrics.fDescent - metrics.fAscent;

    if (!metrics.hasUnderlineThickness(&out.underlineThickness)) {
        out.underlineThickness = kDefaultLineHeightToUnderlineThicknessRatio * lineHeight;
    }

    if (!metrics.hasUnderlinePosition(&out.underlinePosition)) {
        out.underlinePosition = kDefaultDescentUnderlineToPositionRatio * metrics.fDescent;
    }

    if (!metrics.hasStrikeoutThickness(&out.strikeoutThickness)) {
        out.strikeoutThickness = kDefaultLineHeightToStrikethroughThicknessRatio * lineHeight;
    }

    if (!metrics.hasStrikeoutPosition(&out.strikeoutPosition)) {
        out.strikeoutPosition =
            (kDefaultAscentToStrikethroughPositionRatio * metrics.fAscent) - out.strikeoutThickness / 2.0f;
    }

    _fontMetricsBySize[fontSize] = out;

    return out;
}

} // namespace snap::drawing
