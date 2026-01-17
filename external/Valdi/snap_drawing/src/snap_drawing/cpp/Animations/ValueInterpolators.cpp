//
//  ValueInterpolators.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/3/20.
//

#include "snap_drawing/cpp/Animations/ValueInterpolators.hpp"
#include <cmath>

namespace snap::drawing {

Rect interpolateValue(const Rect& from, const Rect& to, double ratio) {
    auto l = interpolateValue(from.left, to.left, ratio);
    auto t = interpolateValue(from.top, to.top, ratio);
    auto r = interpolateValue(from.right, to.right, ratio);
    auto b = interpolateValue(from.bottom, to.bottom, ratio);
    return Rect::makeLTRB(l, t, r, b);
}

Size interpolateValue(const Size& from, const Size& to, double ratio) {
    auto width = interpolateValue(from.width, to.width, ratio);
    auto height = interpolateValue(from.height, to.height, ratio);
    return Size::make(width, height);
}

Point interpolateValue(const Point& from, const Point& to, double ratio) {
    auto x = interpolateValue(from.x, to.x, ratio);
    auto y = interpolateValue(from.y, to.y, ratio);
    return Point::make(x, y);
}

Scalar interpolateValue(Scalar from, Scalar to, double ratio) {
    return from + ((to - from) * ratio);
}

static ColorComponent blendColorAlpha(ColorComponent from, ColorComponent to, double ratio) {
    auto alpha = (1.0 - ratio) * static_cast<double>(from) + (ratio * static_cast<double>(to));
    return static_cast<ColorComponent>(alpha);
}

static ColorComponent blendColorComponent(ColorComponent from, ColorComponent to, double ratio) {
    auto component = sqrt((1.0 - ratio) * pow(static_cast<double>(from), 2) + ratio * pow(static_cast<double>(to), 2));

    return static_cast<ColorComponent>(component);
}

Color interpolateValue(Color from, Color to, double ratio) {
    double clampedRatio = std::clamp(ratio, 0.0, 1.0);
    auto alpha = blendColorAlpha(from.getAlpha(), to.getAlpha(), clampedRatio);
    auto red = blendColorComponent(from.getRed(), to.getRed(), clampedRatio);
    auto green = blendColorComponent(from.getGreen(), to.getGreen(), clampedRatio);
    auto blue = blendColorComponent(from.getBlue(), to.getBlue(), clampedRatio);

    return Color::makeARGB(alpha, red, green, blue);
}

static inline Scalar interpolateCorner(
    Scalar from, Scalar to, bool fromIsPercent, bool toIsPercent, double progress, Scalar sideLength) {
    if (fromIsPercent == toIsPercent) {
        return interpolateValue(from, to, progress);
    } else if (fromIsPercent) {
        auto fromPoints = from * sideLength / 100;
        return interpolateValue(fromPoints, to, progress);
    } else {
        auto fromPercent = from / sideLength * 100;
        return interpolateValue(fromPercent, to, progress);
    }
}

BorderRadius interpolateBorderRadius(BorderRadius from, BorderRadius to, const Rect& bounds, double ratio) {
    Scalar sideLength = BorderRadius::sideLengthForPercentages(bounds);
    return BorderRadius(
        interpolateCorner(
            from.topLeft(), to.topLeft(), from.topLeftIsPercent(), to.topLeftIsPercent(), ratio, sideLength),
        interpolateCorner(
            from.topRight(), to.topRight(), from.topRightIsPercent(), to.topRightIsPercent(), ratio, sideLength),
        interpolateCorner(from.bottomRight(),
                          to.bottomRight(),
                          from.bottomRightIsPercent(),
                          to.bottomRightIsPercent(),
                          ratio,
                          sideLength),
        interpolateCorner(from.bottomLeft(),
                          to.bottomLeft(),
                          from.bottomLeftIsPercent(),
                          to.bottomLeftIsPercent(),
                          ratio,
                          sideLength),
        to.topLeftIsPercent(),
        to.topRightIsPercent(),
        to.bottomRightIsPercent(),
        to.bottomLeftIsPercent());
}

} // namespace snap::drawing
