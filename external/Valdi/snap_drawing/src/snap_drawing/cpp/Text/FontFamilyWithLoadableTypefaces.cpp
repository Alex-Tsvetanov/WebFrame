//
//  FontFamilyWithLoadableTypefaces.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 12/12/22.
//

#include "snap_drawing/cpp/Text/FontFamilyWithLoadableTypefaces.hpp"
#include "snap_drawing/cpp/Text/LoadableTypeface.hpp"
#include "snap_drawing/cpp/Text/Typeface.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace snap::drawing {

FontFamilyWithLoadableTypefaces::FontFamilyWithLoadableTypefaces(Valdi::ILogger& logger, const String& familyName)
    : FontFamily(familyName), _logger(logger) {}
FontFamilyWithLoadableTypefaces::~FontFamilyWithLoadableTypefaces() = default;

void FontFamilyWithLoadableTypefaces::registerLoadableTypeface(FontStyle fontStyle,
                                                               const Ref<LoadableTypeface>& loadableTypeface) {
    _typefaces[fontStyle] = loadableTypeface;
}

void FontFamilyWithLoadableTypefaces::unregisterLoadableTypeface(const Ref<LoadableTypeface>& loadableTypeface) {
    auto it = _typefaces.begin();
    while (it != _typefaces.end()) {
        if (it->second == loadableTypeface) {
            it = _typefaces.erase(it);
        } else {
            it++;
        }
    }
}

static int computeSelectionScore(FontStyle desiredFontStyle, FontStyle candidateFontStyle) {
    static constexpr int kMismatchedSlantOffset = static_cast<int>(FontWeightExtraBlack) + 1;

    /**
     * Taken from Valdi's FontManager.kt:
     * This will return a score where values closer than 0 are higher matches.
     * In case of equality, values lower than 0 should be prioritized over values higher than 0.
     *
     * Examples:
     * Desired LIGHT
     * CANDIDATE: DEMI_BOLD
     * Score: -3
     * Score is negative because weight is lower than the desired weight
     *
     * Desired: MEDIUM
     * CANDIDATE: NORMAL
     * Score: 1
     * Score is positive because weight is higher than the desired weight

     * Desired: NORMAL
     * Candidate: NORMAL
     * Mismatched style
     * Score: 8
     * Mismatching styles look very bad, we prioritize matching styles over weights.
     *
     */

    auto score = static_cast<int>(candidateFontStyle.getWeight()) - static_cast<int>(desiredFontStyle.getWeight());
    if (desiredFontStyle.getSlant() != candidateFontStyle.getSlant()) {
        if (score >= 0) {
            score += kMismatchedSlantOffset;
        } else {
            score -= kMismatchedSlantOffset;
        }
    }

    // TODO(simon): Also look at FontWidth.

    return score;
}

Ref<LoadableTypeface> FontFamilyWithLoadableTypefaces::resolveLoadableTypeface(FontStyle fontStyle) const {
    std::optional<std::pair<FontStyle, Ref<LoadableTypeface>>> bestLoadableTypeface;

    for (const auto& it : _typefaces) {
        if (!bestLoadableTypeface) {
            bestLoadableTypeface = {std::make_pair(it.first, it.second)};
        } else {
            /**
             Algorithm taken from Valdi's FontManager.kt
             */
            auto previousScore = computeSelectionScore(fontStyle, bestLoadableTypeface.value().first);
            auto newScore = computeSelectionScore(fontStyle, it.first);

            auto previousScoreAbs = std::abs(previousScore);
            auto newScoreAbs = std::abs(newScore);

            // We are looking for the closest value to zero
            if (newScoreAbs < previousScoreAbs) {
                bestLoadableTypeface = {std::make_pair(it.first, it.second)};
            } else if (newScoreAbs == previousScoreAbs) {
                // In case of equality, we choose the lower values, which means we prioritize
                // weight which are less strong than our requested weight over the ones
                // which are higher
                if (newScore < previousScore) {
                    bestLoadableTypeface = {std::make_pair(it.first, it.second)};
                }
            }
        }
    }

    if (!bestLoadableTypeface) {
        return nullptr;
    }

    return std::move(bestLoadableTypeface.value().second);
}

Ref<Typeface> FontFamilyWithLoadableTypefaces::matchStyle(SkFontMgr& fontMgr, FontStyle fontStyle) {
    while (!_typefaces.empty()) {
        auto loadableTypeface = resolveLoadableTypeface(fontStyle);
        if (loadableTypeface == nullptr) {
            return nullptr;
        }

        const auto& result = loadableTypeface->get(fontMgr);
        if (result) {
            return result.value();
        }

        VALDI_ERROR(
            _logger, "Failed to load registered Typeface in font family '{}': {}", getFamilyName(), result.error());
        unregisterLoadableTypeface(loadableTypeface);
    }

    return nullptr;
}

} // namespace snap::drawing
