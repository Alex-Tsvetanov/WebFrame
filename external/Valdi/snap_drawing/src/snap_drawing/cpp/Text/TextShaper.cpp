//
//  TextShaper.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 3/8/22.
//

#include "snap_drawing/cpp/Text/TextShaper.hpp"
#include "snap_drawing/cpp/Text/TextShaperHarfbuzz.hpp"
#include "snap_drawing/cpp/Text/WordCachingTextShaper.hpp"

namespace snap::drawing {

Ref<TextShaper> TextShaper::make(bool enableCache) {
    auto strategy = enableCache ? WordCachingTextShaperStrategy::PrioritizeCorrectness :
                                  WordCachingTextShaperStrategy::DisableCache;
    auto harfbuzzShaper = Valdi::makeShared<TextShaperHarfbuzz>();
    return Valdi::makeShared<WordCachingTextShaper>(harfbuzzShaper, strategy);
}

} // namespace snap::drawing
