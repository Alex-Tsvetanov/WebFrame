//
//  ImageFilter.hpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 5/2/22.
//

#pragma once

#include <optional>

#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

class ImageFilter : public ValdiObject {
public:
    static constexpr size_t kColorMatrixSize = 20;

    ImageFilter();

    VALDI_CLASS_HEADER(ImageFilter)

    float getBlurRadius() const;
    void setBlurRadius(float blurRadius);
    Valdi::Ref<ImageFilter> withBlurRadius(float radius) const;

    float getColorMatrixComponent(size_t index) const;
    const float* getColorMatrix() const;

    bool isIdentityColorMatrix() const;

    void concatColorMatrix(const float* colorMatrix);

private:
    float _colorMatrix[kColorMatrixSize];
    float _blurRadius = 0.0f;
};

} // namespace Valdi
