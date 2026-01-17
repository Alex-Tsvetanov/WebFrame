//
//  AnimatedImageLayer.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 7/15/22.
//

#include "snap_drawing/cpp/Layers/AnimatedImageLayer.hpp"
#include "snap_drawing/cpp/Utils/AnimatedImage.hpp"

namespace snap::drawing {

ImageAnimation::ImageAnimation() = default;
ImageAnimation::~ImageAnimation() = default;

bool ImageAnimation::run(Layer& layer, Duration delta) {
    return dynamic_cast<AnimatedImageLayer&>(layer).advanceTime(delta);
}

void ImageAnimation::cancel(Layer& /*layer*/) {}

void ImageAnimation::complete(Layer& /*layer*/) {}

void ImageAnimation::addCompletion(AnimationCompletion&& /*completion*/) {
    // no-op
}

AnimatedImageLayer::AnimatedImageLayer(const Ref<Resources>& resources) : Layer(resources) {}
AnimatedImageLayer::~AnimatedImageLayer() = default;

void AnimatedImageLayer::onInitialize() {
    Layer::onInitialize();

    _animation = Valdi::makeShared<ImageAnimation>();
}

void AnimatedImageLayer::onDraw(DrawingContext& drawingContext) {
    if (_image == nullptr) {
        return;
    }

    auto drawBounds = drawingContext.drawBounds();

    auto imageWidth = static_cast<Scalar>(_image->getSize().width);
    auto imageHeight = static_cast<Scalar>(_image->getSize().height);
    auto imageRect = Rect::makeLTRB(0, 0, imageWidth, imageHeight);

    auto imageDrawBounds = drawBounds.makeFittingSize(imageRect.size(), _fittingSizeMode);

    // Clip to view bounds. Applied before matrix transform
    auto drawRect = imageDrawBounds.intersection(drawBounds);
    drawingContext.clipRect(drawRect);

    if (_shouldFlip) {
        drawingContext.concat(Matrix::makeScaleTranslate(-1, 1, imageDrawBounds.width(), 0));
    }

    _image->draw(drawingContext.canvas(), imageDrawBounds, _currentTime, _fittingSizeMode);
}

void AnimatedImageLayer::onLoadedAssetChanged(const Ref<Valdi::LoadedAsset>& loadedAsset, bool shouldDrawFlipped) {
    setImage(Valdi::castOrNull<AnimatedImage>(loadedAsset));
    setShouldFlip(shouldDrawFlipped);
}

void AnimatedImageLayer::setFittingSizeMode(FittingSizeMode fittingSizeMode) {
    if (_fittingSizeMode != fittingSizeMode) {
        _fittingSizeMode = fittingSizeMode;
        setNeedsDisplay();
    }
}

void AnimatedImageLayer::setImage(const Ref<AnimatedImage>& image) {
    if (_image != image) {
        auto hadImage = _image != nullptr;
        _image = image;

        updateAnimationTimeWindow();
        updateActiveAnimation();
        if (!hadImage) {
            setCurrentTime(_currentTime, false, true);
        } else {
            setCurrentTime(Duration(), false, true);
        }
        setNeedsDisplay();
    }
}

const Ref<AnimatedImage>& AnimatedImageLayer::getImage() const {
    return _image;
}

void AnimatedImageLayer::setAdvanceRate(double advanceRate) {
    if (_advanceRate != advanceRate) {
        _advanceRate = advanceRate;
        updateActiveAnimation();
    }
}

void AnimatedImageLayer::setShouldLoop(bool shouldLoop) {
    _shouldLoop = shouldLoop;
}

void AnimatedImageLayer::setShouldFlip(bool shouldFlip) {
    if (_shouldFlip != shouldFlip) {
        _shouldFlip = shouldFlip;
        setNeedsDisplay();
    }
}

void AnimatedImageLayer::setCurrentTime(const Duration& currentTime) {
    setCurrentTime(currentTime, true, false);
}

void AnimatedImageLayer::setCurrentTime(const Duration& currentTime, bool relative, bool forceNotify) {
    bool shouldNotify = forceNotify;

    Duration newTime;
    if (_image == nullptr) {
        newTime = currentTime;
    } else if (relative) {
        if (_shouldLoop) {
            auto duration = _clampedEndTime - _clampedStartTime;
            newTime = currentTime % duration + _clampedStartTime;
        } else {
            newTime = std::clamp(_clampedStartTime + currentTime, _clampedStartTime, _clampedEndTime);
        }
    } else {
        if (_shouldLoop) {
            auto duration = getDuration();
            newTime = std::clamp(currentTime % duration, _clampedStartTime, _clampedEndTime);
            ;
        } else {
            newTime = std::clamp(currentTime, _clampedStartTime, _clampedEndTime);
        }
    }

    if (_currentTime != newTime) {
        _currentTime = newTime;
        shouldNotify = true;
        setNeedsDisplay();
    }
    if (_listener != nullptr && shouldNotify) {
        _listener->onProgress(*this, _currentTime, getDuration());
    }
}

bool AnimatedImageLayer::advanceTime(Duration delta) {
    auto duration = _clampedEndTime - _clampedStartTime;
    auto newTime = _currentTime + Duration(delta.seconds() * _advanceRate) - _clampedStartTime;
    bool reachedEnd = false;

    if (duration <= Duration()) {
        newTime = Duration();
        reachedEnd = !_shouldLoop;
    } else if (newTime > duration) {
        if (_shouldLoop) {
            newTime = newTime % duration;
        } else {
            newTime = duration;
            reachedEnd = true;
        }
    } else if (newTime < Duration()) {
        if (_shouldLoop) {
            newTime = duration + (newTime % duration);
        } else {
            newTime = Duration();
            reachedEnd = true;
        }
    }

    newTime += _clampedStartTime;
    setCurrentTime(newTime, false, false);

    return reachedEnd;
}

void AnimatedImageLayer::setAnimationStartTime(const Duration& startTime) {
    _animationStartTime = startTime;
    updateAnimationTimeWindow();
}

void AnimatedImageLayer::setAnimationEndTime(const Duration& endTime) {
    _animationEndTime = endTime;
    updateAnimationTimeWindow();
}

void AnimatedImageLayer::onRootChanged(ILayerRoot* root) {
    Layer::onRootChanged(root);

    updateActiveAnimation();
}

void AnimatedImageLayer::updateActiveAnimation() {
    static auto kAnimationKey = STRING_LITERAL("imgAnim");

    if (getRoot() == nullptr || _image == nullptr || _advanceRate == 0.0) {
        removeAnimation(kAnimationKey);
    } else if (!hasAnimation(kAnimationKey)) {
        addAnimation(kAnimationKey, _animation);
    }
}

void AnimatedImageLayer::setListener(const Ref<AnimatedImageLayerListener>& listener) {
    _listener = listener;
}

const Ref<AnimatedImageLayerListener>& AnimatedImageLayer::getListener() const {
    return _listener;
}

void AnimatedImageLayer::updateAnimationTimeWindow() {
    if (_image == nullptr) {
        _clampedStartTime = Duration();
        _clampedEndTime = Duration();
        return;
    }

    auto duration = getDuration();
    _clampedStartTime = std::clamp(_animationStartTime, Duration(), duration);
    _clampedEndTime = _animationEndTime > Duration() ? std::clamp(_animationEndTime, Duration(), duration) : duration;

    if (_clampedEndTime < _clampedStartTime) {
        _clampedEndTime = _clampedStartTime;
    }
}

Duration AnimatedImageLayer::getDuration() const {
    return _image != nullptr ? _image->getDuration() : Duration();
}

} // namespace snap::drawing
