#pragma once

#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"
#include "snap_drawing/cpp/Drawing/LinearGradient.hpp"
#include "snap_drawing/cpp/Drawing/RadialGradient.hpp"
#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/Color.hpp"
#include "snap_drawing/cpp/Utils/Scalar.hpp"

#include <vector>

namespace snap::drawing {

class GradientWrapper {
public:
    enum class GradientType { LINEAR, RADIAL };

    GradientWrapper();

    void update(const Rect& bounds);

    void applyToPaint(Paint& paint) const;

    void draw(DrawingContext& drawingContext, const BorderRadius& borderRadius);

    void setAsLinear(std::vector<Scalar>&& locations,
                     std::vector<Color>&& colors,
                     LinearGradientOrientation orientation);
    void setAsRadial(std::vector<Scalar>&& locations, std::vector<Color>&& colors);

    void clear();
    bool clearIfNeeded(GradientType type);

    bool isDirty();
    bool hasGradient();

    Ref<LinearGradient> getLinearGradient();
    Ref<RadialGradient> getRadialGradient();

private:
    Ref<LinearGradient> _linearGradient;
    Ref<RadialGradient> _radialGradient;
};
} // namespace snap::drawing