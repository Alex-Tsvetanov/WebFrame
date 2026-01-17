//
//  VideoLayer.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 6/16/22.
//

#include "snap_drawing/cpp/Layers/VideoLayer.hpp"
#include "snap_drawing/cpp/Utils/ImageQueue.hpp"

namespace snap::drawing {

VideoPlaybackAnimation::VideoPlaybackAnimation(const Ref<ImageQueue>& imageQueue, const Ref<ImageLayer>& imageLayer)
    : _imageQueue(imageQueue), _imageLayer(imageLayer) {}

VideoPlaybackAnimation::~VideoPlaybackAnimation() = default;

bool VideoPlaybackAnimation::run(Layer& /*layer*/, Duration /*delta*/) {
    auto nextImage = _imageQueue->dequeue();
    if (nextImage) {
        _imageLayer->setImage(nextImage.value());
    }
    return false;
}

void VideoPlaybackAnimation::cancel(Layer& layer) {}

void VideoPlaybackAnimation::complete(Layer& layer) {}

void VideoPlaybackAnimation::addCompletion(AnimationCompletion&& completion) {}

VideoLayer::VideoLayer(const Ref<Resources>& resources) : Layer(resources) {}

VideoLayer::~VideoLayer() = default;

void VideoLayer::onInitialize() {
    Layer::onInitialize();

    _imageLayer = makeLayer<ImageLayer>(getResources());
    _imageLayer->setOptimizeRenderingForFrequentUpdates(true);
    addChild(_imageLayer);
}

void VideoLayer::onRootChanged(ILayerRoot* root) {
    Layer::onRootChanged(root);

    updatePlaybackAnimation();
}

void VideoLayer::onBoundsChanged() {
    Layer::onBoundsChanged();

    _imageLayer->setFrame(Rect::makeXYWH(0, 0, getFrame().width(), getFrame().height()));
}

void VideoLayer::setFittingSizeMode(FittingSizeMode fittingSizeMode) {
    _imageLayer->setFittingSizeMode(fittingSizeMode);
}

void VideoLayer::updatePlaybackAnimation() {
    static auto kAnimationKey = STRING_LITERAL("videoPlayback");

    if (getRoot() != nullptr && _imageQueue != nullptr) {
        addAnimation(kAnimationKey, Valdi::makeShared<VideoPlaybackAnimation>(_imageQueue, _imageLayer));
    } else {
        removeAnimation(kAnimationKey);
    }
}

void VideoLayer::setImageQueue(const Ref<ImageQueue>& imageQueue) {
    if (_imageQueue != imageQueue) {
        _imageQueue = imageQueue;
        updatePlaybackAnimation();
    }
}

const Ref<ImageQueue>& VideoLayer::getImageQueue() const {
    return _imageQueue;
}

} // namespace snap::drawing
