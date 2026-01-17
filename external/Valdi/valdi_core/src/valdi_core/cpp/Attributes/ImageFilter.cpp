//
//  ImageFilter.cpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 5/2/22.
//

#include "valdi_core/cpp/Attributes/ImageFilter.hpp"

namespace Valdi {

static void makeIdentity(float* colorMatrix) {
    std::memset(colorMatrix, 0, sizeof(float) * ImageFilter::kColorMatrixSize);

    colorMatrix[0] = 1.0f;
    colorMatrix[6] = 1.0f;
    colorMatrix[12] = 1.0f;
    colorMatrix[18] = 1.0f;
}

ImageFilter::ImageFilter() {
    makeIdentity(_colorMatrix);
}

VALDI_CLASS_IMPL(ImageFilter)

float ImageFilter::getBlurRadius() const {
    return _blurRadius;
}

void ImageFilter::setBlurRadius(float blurRadius) {
    _blurRadius = blurRadius;
}

Valdi::Ref<ImageFilter> ImageFilter::withBlurRadius(float radius) const {
    auto retval = makeShared<ImageFilter>();
    retval->setBlurRadius(radius);
    std::memcpy(retval->_colorMatrix, _colorMatrix, sizeof(_colorMatrix));
    return retval;
}

float ImageFilter::getColorMatrixComponent(size_t index) const {
    return _colorMatrix[index];
}

void ImageFilter::concatColorMatrix(const float* colorMatrix) {
    float newColorMatrix[kColorMatrixSize];

    const auto* a = colorMatrix;
    const auto* b = _colorMatrix;
    size_t index = 0;
    for (size_t j = 0; j < kColorMatrixSize; j += 5) {
        for (size_t i = 0; i < 4; i++) {
            newColorMatrix[index++] =
                a[j + 0] * b[i + 0] + a[j + 1] * b[i + 5] + a[j + 2] * b[i + 10] + a[j + 3] * b[i + 15];
        }
        newColorMatrix[index++] = a[j + 0] * b[4] + a[j + 1] * b[9] + a[j + 2] * b[14] + a[j + 3] * b[19] + a[j + 4];
    }

    std::memcpy(_colorMatrix, newColorMatrix, sizeof(newColorMatrix));
}

const float* ImageFilter::getColorMatrix() const {
    return _colorMatrix;
}

bool ImageFilter::isIdentityColorMatrix() const {
    float identityMatrix[ImageFilter::kColorMatrixSize];
    makeIdentity(identityMatrix);

    for (size_t i = 0; i < ImageFilter::kColorMatrixSize; i++) {
        if (identityMatrix[i] != _colorMatrix[i]) {
            return false;
        }
    }

    return true;
}

} // namespace Valdi
