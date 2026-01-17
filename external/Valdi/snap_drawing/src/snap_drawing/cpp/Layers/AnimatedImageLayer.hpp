//
//  AnimatedImageLayer.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 7/15/22.
//

#pragma once

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Layers/Interfaces/ILoadedAssetLayer.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Utils/AnimatedImage.hpp"
#include "valdi_core/Cancelable.hpp"

namespace snap::drawing {

class ImageAnimation : public IAnimation {
public:
    ImageAnimation();
    ~ImageAnimation() override;

    bool run(Layer& layer, Duration delta) override;

    void cancel(Layer& layer) override;

    void complete(Layer& layer) override;

    void addCompletion(AnimationCompletion&& completion) override;
};

class AnimatedImageLayer;

class AnimatedImageLayerListener : public Valdi::SimpleRefCountable {
public:
    virtual void onProgress(AnimatedImageLayer& AnimatedImageLayer, const Duration& time, const Duration& duration) = 0;
};

class AnimatedImageLayer : public Layer, public ILoadedAssetLayer {
public:
    explicit AnimatedImageLayer(const Ref<Resources>& resources);
    ~AnimatedImageLayer() override;

    void onInitialize() override;

    void setImage(const Ref<AnimatedImage>& image);
    const Ref<AnimatedImage>& getImage() const;

    void setShouldFlip(bool shouldFlip);

    void setAdvanceRate(double advanceRate);
    void setShouldLoop(bool shouldLoop);
    void setCurrentTime(const Duration& currentTime);
    void setAnimationStartTime(const Duration& startTime);
    void setAnimationEndTime(const Duration& endTime);

    bool advanceTime(Duration delta);

    void setListener(const Ref<AnimatedImageLayerListener>& listener);
    const Ref<AnimatedImageLayerListener>& getListener() const;

    void onLoadedAssetChanged(const Ref<Valdi::LoadedAsset>& loadedAsset, bool shouldDrawFlipped) override;

    void setFittingSizeMode(FittingSizeMode fittingSizeMode);

protected:
    void onDraw(DrawingContext& drawingContext) override;
    void onRootChanged(ILayerRoot* root) override;

private:
    Ref<AnimatedImage> _image;
    Ref<ImageAnimation> _animation;
    Ref<AnimatedImageLayerListener> _listener;
    Duration _currentTime;
    Duration _animationStartTime;
    Duration _animationEndTime;
    Duration _clampedStartTime;
    Duration _clampedEndTime;
    bool _shouldLoop = false;
    bool _shouldFlip = false;
    double _advanceRate = 0.0;
    FittingSizeMode _fittingSizeMode = FittingSizeModeCenterScaleFit;

    void updateActiveAnimation();
    void setCurrentTime(const Duration& currentTime, bool relative, bool forceNotify);

    Duration getDuration() const;
    void updateAnimationTimeWindow();
};

} // namespace snap::drawing
