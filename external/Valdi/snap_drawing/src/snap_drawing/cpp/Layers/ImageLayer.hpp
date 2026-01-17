//
//  ImageLayer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/28/20.
//

#pragma once

#include "snap_drawing/cpp/Layers/Interfaces/ILoadedAssetLayer.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"

namespace Valdi {
class Asset;
}

namespace snap::drawing {

class Image;
class ValdiAnimator;

class ImageLayer : public Layer, public ILoadedAssetLayer {
public:
    explicit ImageLayer(const Ref<Resources>& resources);
    ~ImageLayer() override;

    void setImage(const Ref<Image>& image);
    void setTintColor(Color tintColor);
    void setShouldFlip(bool shouldFlip);

    void setContentScaleX(Scalar contentScaleX);
    void setContentScaleY(Scalar contentScaleY);
    void setContentRotation(Scalar contentRotation);

    void setFittingSizeMode(FittingSizeMode fittingSizeMode);

    void setOptimizeRenderingForFrequentUpdates(bool optimizeRenderingForFrequentUpdates);

    void onLoadedAssetChanged(const Ref<Valdi::LoadedAsset>& loadedAsset, bool shouldDrawFlipped) override;

protected:
    void onDraw(DrawingContext& drawingContext) override;

private:
    Ref<Image> _image;
    Color _tintColor;
    Paint _imagePaint;
    FittingSizeMode _fittingSizeMode = FittingSizeModeFill;
    bool _shouldFlip = false;
    bool _optimizeRenderingForFrequentUpdates = false;

    Scalar _contentScaleX = 1;
    Scalar _contentScaleY = 1;
    Scalar _contentRotation = 0;

    void handleAssetLoaded(const Ref<Valdi::Asset>& asset, const Valdi::Result<Ref<Image>>& result);
};

} // namespace snap::drawing
