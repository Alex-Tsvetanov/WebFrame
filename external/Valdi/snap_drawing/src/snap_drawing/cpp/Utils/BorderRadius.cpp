//
//  BorderRadius.cpp
//  valdi-skia-app
//
//  Created by Simon Corsin on 6/28/20.
//

#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include <algorithm>
#include <fmt/format.h>

namespace snap::drawing {

BorderRadius::BorderRadius(Scalar topLeft,
                           Scalar topRight,
                           Scalar bottomRight,
                           Scalar bottomLeft,
                           bool topLeftIsPercent,
                           bool topRightIsPercent,
                           bool bottomRightIsPercent,
                           bool bottomLeftIsPercent)
    : _topLeft(topLeft),
      _topRight(topRight),
      _bottomRight(bottomRight),
      _bottomLeft(bottomLeft),
      _topLeftIsPercent(topLeftIsPercent),
      _topRightIsPercent(topRightIsPercent),
      _bottomRightIsPercent(bottomRightIsPercent),
      _bottomLeftIsPercent(bottomLeftIsPercent) {
    _isEmpty = topLeft == 0 && topRight == 0 && bottomRight == 0 && bottomLeft == 0;
}

BorderRadius::BorderRadius() = default;

bool BorderRadius::isEmpty() const {
    return _isEmpty;
}

Scalar BorderRadius::sideLengthForPercentages(const Rect& bounds) {
    return std::min(bounds.width(), bounds.height());
}

BorderRadius BorderRadius::makeCircle() {
    return BorderRadius::makeOval(50, true);
}

BorderRadius BorderRadius::makeOval(Scalar corners, bool isPercent) {
    return BorderRadius(corners, corners, corners, corners, isPercent, isPercent, isPercent, isPercent);
}

static inline void populateBorderRadius(Scalar* radii, Scalar sizeRatio, Scalar value, bool isPercent) {
    if (isPercent) {
        value *= sizeRatio;
    }
    radii[0] = value;
    radii[1] = value;
}

Path BorderRadius::getPath(const Rect& bounds) const {
    Path path;

    applyToPath(bounds, path);

    return path;
}

void BorderRadius::applyToPath(const Rect& bounds, Path& path) const {
    Scalar radii[8];

    Scalar sizeRatio = sideLengthForPercentages(bounds) / 100;

    populateBorderRadius(&radii[0], sizeRatio, _topLeft, _topLeftIsPercent);
    populateBorderRadius(&radii[2], sizeRatio, _topRight, _topRightIsPercent);
    populateBorderRadius(&radii[4], sizeRatio, _bottomRight, _bottomRightIsPercent);
    populateBorderRadius(&radii[6], sizeRatio, _bottomLeft, _bottomLeftIsPercent);

    path.addRoundRect(bounds, radii, true);
}

bool BorderRadius::operator==(const BorderRadius& other) const {
    return _topLeft == other._topLeft && _topRight == other._topRight && _bottomRight == other._bottomRight &&
           _bottomLeft == other._bottomLeft && _topLeftIsPercent == other._topLeftIsPercent &&
           _topRightIsPercent == other._topRightIsPercent && _bottomRightIsPercent == other._bottomRightIsPercent &&
           _bottomLeftIsPercent == other._bottomLeftIsPercent;
}

bool BorderRadius::operator!=(const BorderRadius& other) const {
    return !(*this == other);
}

static std::string borderToString(Scalar value, bool isPercent) {
    auto strValue = fmt::format("{:.1f}", value);
    if (isPercent) {
        strValue += "%";
    }
    return strValue;
}

std::string BorderRadius::toString() const {
    return fmt::format("[{}, {}, {}, {}]",
                       borderToString(_topLeft, _topLeftIsPercent),
                       borderToString(_topRight, _topRightIsPercent),
                       borderToString(_bottomRight, _bottomRightIsPercent),
                       borderToString(_bottomLeft, _bottomLeftIsPercent));
}

std::ostream& operator<<(std::ostream& os, const BorderRadius& borderRadius) {
    return os << borderRadius.toString();
}

} // namespace snap::drawing
