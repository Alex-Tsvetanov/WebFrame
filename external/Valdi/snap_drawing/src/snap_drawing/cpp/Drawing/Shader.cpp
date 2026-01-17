//
//  Shader.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/25/22.
//

#include "snap_drawing/cpp/Drawing/Shader.hpp"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkTileMode.h"
#include "snap_drawing/cpp/Utils/Image.hpp"

namespace snap::drawing {

static SkSamplingOptions makeSmaplingOptions(FilterQuality quality) {
    switch (quality) {
        case FilterQualityNone:
            return SkSamplingOptions();
        case FilterQualityLow:
            return SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone);
        case FilterQualityMedium:
            return SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear);
        case FilterQualityHigh:
            return SkSamplingOptions(SkCubicResampler::Mitchell());
    }
}

Shader::Shader() : Shader(nullptr) {}

Shader::Shader(sk_sp<SkShader> skValue) : WrappedSkValue<sk_sp<SkShader>>(std::move(skValue)) {}
Shader::~Shader() = default;

Shader Shader::withLocalMatrix(const Matrix& localMatrix) const {
    return Shader(getSkValue()->makeWithLocalMatrix(localMatrix.getSkValue()));
}

Shader Shader::makeEmpty() {
    return Shader(SkShaders::Empty());
}

Shader Shader::makeColor(Color color) {
    return Shader(SkShaders::Color(color.getSkValue()));
}

Shader Shader::makeBlend(BlendMode mode, Shader dst, Shader src) {
    return Shader(SkShaders::Blend(toSkBlendMode(mode), dst.getSkValue(), src.getSkValue()));
}

Shader Shader::makeImage(const Ref<Image>& image, const Matrix* localMatrix, FilterQuality filterQuality) {
    auto samplingOptions = makeSmaplingOptions(filterQuality);
    const auto* skMatrix = localMatrix != nullptr ? &localMatrix->getSkValue() : nullptr;
    return Shader(image->getSkValue()->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, samplingOptions, skMatrix));
}

} // namespace snap::drawing
