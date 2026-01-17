//
//  ImageLayer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/28/20.
//

#include "snap_drawing/cpp/Layers/ImageLayer.hpp"

#include "snap_drawing/cpp/Drawing/Shader.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"

#include "valdi_core/cpp/Resources/Asset.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"

#include "valdi_core/AssetLoadObserver.hpp"
#include "valdi_core/Platform.hpp"

#include "include/core/SkColorFilter.h"
#include "include/effects/SkImageFilters.h"

namespace snap::drawing {

constexpr Scalar kUIKitToSkiaBlurRatio = 2.0f;

ImageLayer::ImageLayer(const Ref<Resources>& resources) : Layer(resources), _tintColor(Color::transparent()) {
    _imagePaint.setAntiAlias(true);
}

ImageLayer::~ImageLayer() = default;

// From https://github.com/servo/skia/blob/master/src/effects/SkBlurMask.cpp#L23
static const SkScalar kBlurSigmaScale = 0.57735f;

static Scalar radiusToSigma(Scalar radius) {
    return radius > 0 ? kBlurSigmaScale * radius + 0.5f : 0.0f;
}

void ImageLayer::onDraw(DrawingContext& drawingContext) {
    Layer::onDraw(drawingContext);

    if (_image == nullptr) {
        return;
    }

    auto drawBounds = drawingContext.drawBounds();

    auto imageWidth = static_cast<Scalar>(_image->width());
    auto imageHeight = static_cast<Scalar>(_image->height());

    if (imageWidth == 0 || imageHeight == 0) {
        return;
    }

    auto imageRect = Rect::makeLTRB(0, 0, imageWidth, imageHeight);
    auto imageDrawBounds = drawBounds.makeFittingSize(imageRect.size(), _fittingSizeMode);

    if (_contentScaleX != 1.0) {
        auto previousWidth = imageDrawBounds.width();
        auto offsetX = (previousWidth - previousWidth * _contentScaleX) / 2.0;
        imageDrawBounds.left += offsetX;
        imageDrawBounds.right -= offsetX;
    }

    if (_contentScaleY != 1.0) {
        auto previousHeight = imageDrawBounds.height();
        auto offsetY = (previousHeight - previousHeight * _contentScaleY) / 2.0;
        imageDrawBounds.top += offsetY;
        imageDrawBounds.bottom -= offsetY;
    }

    auto drawRect = imageDrawBounds.intersection(drawBounds);

    auto drawWidth = imageDrawBounds.width();
    auto drawHeight = imageDrawBounds.height();

    Scalar matrixScaleX = drawWidth / imageWidth;
    Scalar matrixScaleY = drawHeight / imageHeight;
    Scalar matrixTranslateX = imageDrawBounds.left;
    Scalar matrixTranslateY = imageDrawBounds.top;

    if (_shouldFlip) {
        matrixScaleX *= -1.0;
        matrixTranslateX += drawWidth;
    }

    auto localMatrix = Matrix::makeScaleTranslate(matrixScaleX, matrixScaleY, matrixTranslateX, matrixTranslateY);

    if (_contentRotation != 0.0) {
        Scalar centerX = localMatrix.getTranslateX() + drawWidth / 2.0f;
        Scalar centerY = localMatrix.getTranslateY() + drawHeight / 2.0f;
        localMatrix.postRotate(_contentRotation, centerX, centerY);
    }

    auto imagePaint = _imagePaint;

    const auto& filter = _image->getFilter();

    if (filter != nullptr) {
        auto& skPaint = imagePaint.getSkValue();
        if (!filter->isIdentityColorMatrix()) {
            auto colorMatrixFilter = SkColorFilters::Matrix(filter->getColorMatrix());
            if (skPaint.getColorFilter() == nullptr) {
                skPaint.setColorFilter(std::move(colorMatrixFilter));
            } else {
                skPaint.setColorFilter(SkColorFilters::Compose(std::move(colorMatrixFilter), skPaint.refColorFilter()));
            }
        }

        if (filter->getBlurRadius() != 0.0f) {
            auto blurSigma = radiusToSigma(filter->getBlurRadius()) * kUIKitToSkiaBlurRatio;
            auto xScaleRatio = drawRect.width() / imageWidth;
            auto yScaleRatio = drawRect.height() / imageHeight;
            skPaint.setImageFilter(
                SkImageFilters::Blur(blurSigma * xScaleRatio, blurSigma * yScaleRatio, SkTileMode::kClamp, nullptr));
            drawingContext.clipRect(drawRect);
        }
    }

    if (_optimizeRenderingForFrequentUpdates) {
        if (!getBorderRadius().isEmpty()) {
            drawingContext.clipPath(getBorderRadius().getPath(drawBounds));
        } else {
            drawingContext.clipRect(drawBounds);
        }
        drawingContext.drawImage(*_image, imageRect, imageDrawBounds, &imagePaint);
    } else {
        imagePaint.setShader(Shader::makeImage(_image, &localMatrix, FilterQualityLow));

        if (!getBorderRadius().isEmpty()) {
            auto drawPath = getBorderRadius().getPath(drawBounds);
            if (drawRect != drawBounds) {
                drawingContext.clipRect(drawRect);
            }

            drawingContext.drawPaint(imagePaint, drawPath);
        } else {
            drawingContext.drawPaint(imagePaint, drawRect);
        }
    }
}

void ImageLayer::onLoadedAssetChanged(const Ref<Valdi::LoadedAsset>& loadedAsset, bool shouldDrawFlipped) {
    setImage(Valdi::castOrNull<Image>(loadedAsset));
    setShouldFlip(shouldDrawFlipped);
}

void ImageLayer::setFittingSizeMode(FittingSizeMode fittingSizeMode) {
    if (_fittingSizeMode != fittingSizeMode) {
        _fittingSizeMode = fittingSizeMode;
        setNeedsDisplay();
    }
}

void ImageLayer::setImage(const Ref<Image>& image) {
    if (_image != image) {
        _image = image;
        setNeedsDisplay();
    }
}

void ImageLayer::setShouldFlip(bool shouldFlip) {
    if (_shouldFlip != shouldFlip) {
        _shouldFlip = shouldFlip;
        setNeedsDisplay();
    }
}

void ImageLayer::setTintColor(Color tintColor) {
    if (_tintColor != tintColor) {
        _tintColor = tintColor;
        _imagePaint.setBlendColorFilter(tintColor);
        setNeedsDisplay();
    }
}

void ImageLayer::setContentScaleX(Scalar contentScaleX) {
    if (_contentScaleX != contentScaleX) {
        _contentScaleX = contentScaleX;
        setNeedsDisplay();
    }
}

void ImageLayer::setContentScaleY(Scalar contentScaleY) {
    if (_contentScaleY != contentScaleY) {
        _contentScaleY = contentScaleY;
        setNeedsDisplay();
    }
}

void ImageLayer::setContentRotation(Scalar contentRotation) {
    if (_contentRotation != contentRotation) {
        _contentRotation = contentRotation;
        setNeedsDisplay();
    }
}

void ImageLayer::setOptimizeRenderingForFrequentUpdates(bool optimizeRenderingForFrequentUpdates) {
    _optimizeRenderingForFrequentUpdates = optimizeRenderingForFrequentUpdates;
}

} // namespace snap::drawing
