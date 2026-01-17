//
//  Shader.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/25/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/BlendMode.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Color.hpp"
#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/SkiaBridge.hpp"

#include "include/core/SkShader.h"

namespace snap::drawing {

class Image;

enum FilterQuality {
    FilterQualityNone,   //!< fastest but lowest quality, typically nearest-neighbor
    FilterQualityLow,    //!< typically bilerp
    FilterQualityMedium, //!< typically bilerp + mipmaps for down-scaling
    FilterQualityHigh,   //!< slowest but highest quality, typically bicubic or better
};

class Shader : public WrappedSkValue<sk_sp<SkShader>> {
public:
    Shader();
    ~Shader();

    Shader withLocalMatrix(const Matrix& localMatrix) const;

    static Shader makeEmpty();
    static Shader makeColor(Color color);
    static Shader makeBlend(BlendMode mode, Shader dst, Shader src);
    static Shader makeImage(const Ref<Image>& image, const Matrix* localMatrix, FilterQuality filterQuality);

private:
    explicit Shader(sk_sp<SkShader> skValue);
};

} // namespace snap::drawing
