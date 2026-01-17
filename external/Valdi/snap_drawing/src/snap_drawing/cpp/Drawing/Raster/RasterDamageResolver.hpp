#pragma once

#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"
#include "snap_drawing/cpp/Utils/Scalar.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include <vector>

namespace snap::drawing {

class DisplayList;
struct ComputeDamageVisitor;

/**
RasterDamageResolver helps with resolving dirty rects from a display list. It is used to
implement delta rasterization, so that only the areas that have changed since the last
drawn frame are rasterized.
 */
class RasterDamageResolver {
public:
    RasterDamageResolver();
    ~RasterDamageResolver();

    void beginUpdates(Scalar surfaceWidth, Scalar surfaceHeight);
    std::vector<Rect> endUpdates();

    void addDamageFromDisplayListUpdates(const DisplayList& displayList);

    void addDamageInRect(const Rect& rect);

private:
    friend ComputeDamageVisitor;
    struct LayerContent {
        Rect absoluteRect;
        Matrix absoluteMatrix;
        Scalar absoluteOpacity;
        Path clipPath;
        bool hasUpdates;
    };

    Scalar _width = 0;
    Scalar _height = 0;
    std::vector<Rect> _damageRects;
    Valdi::FlatMap<uint64_t, LayerContent> _previousLayerContents;
    Valdi::FlatMap<uint64_t, LayerContent> _layerContents;

    void resolveDamage();

    void addNonTransparentLayerInRect(uint64_t layerId,
                                      const Rect& rect,
                                      const Matrix& absoluteMatrix,
                                      const Path& clipPath,
                                      Scalar absoluteOpacity,
                                      bool hasUpdates);
};

} // namespace snap::drawing