//
//  VideoLayer.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 6/16/22.
//

#pragma once

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Layers/ImageLayer.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"

namespace snap::drawing {

class ImageQueue;

class VideoPlaybackAnimation : public IAnimation {
public:
    VideoPlaybackAnimation(const Ref<ImageQueue>& imageQueue, const Ref<ImageLayer>& imageLayer);
    ~VideoPlaybackAnimation() override;

    bool run(Layer& layer, Duration delta) override;

    void cancel(Layer& layer) override;

    void complete(Layer& layer) override;

    void addCompletion(AnimationCompletion&& completion) override;

private:
    Ref<ImageQueue> _imageQueue;
    Ref<ImageLayer> _imageLayer;
};

class VideoLayer : public Layer {
public:
    explicit VideoLayer(const Ref<Resources>& resources);
    ~VideoLayer() override;

    void onInitialize() override;

    void setFittingSizeMode(FittingSizeMode fittingSizeMode);

    void setImageQueue(const Ref<ImageQueue>& imageQueue);
    const Ref<ImageQueue>& getImageQueue() const;

protected:
    void onRootChanged(ILayerRoot* root) override;
    void onBoundsChanged() override;

private:
    Ref<ImageLayer> _imageLayer;
    Ref<ImageQueue> _imageQueue;

    void updatePlaybackAnimation();
};

} // namespace snap::drawing
