//
//  ValdiShapeLayer.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 3/4/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/ShapeLayer.hpp"
#include "valdi_core/cpp/Utils/GeometricPath.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"

namespace snap::drawing {

class ValdiShapeLayer : public ShapeLayer {
public:
    explicit ValdiShapeLayer(const Ref<Resources>& resources);
    ~ValdiShapeLayer() override;

    void setPathData(const Valdi::Ref<Valdi::ValueTypedArray>& pathData);
    const Valdi::Ref<Valdi::ValueTypedArray>& getPathData() const;

protected:
    void onDraw(DrawingContext& drawingContext) override;
    void onBoundsChanged() override;

private:
    Valdi::Ref<Valdi::ValueTypedArray> _pathData;
    bool _pathDirty = false;
};

} // namespace snap::drawing
