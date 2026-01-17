//
//  BorderRadius.cpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 6/24/21.
//

#include "valdi/runtime/Attributes/BorderRadius.hpp"

namespace Valdi {

inline static PercentValue getValue(double value, bool isPercent) {
    return PercentValue(value, isPercent);
}

inline static void setValue(PercentValue value, double& outValue, bool& outPercent) {
    outValue = value.value;
    outPercent = value.isPercent;
}

BorderRadius::BorderRadius() = default;

BorderRadius::BorderRadius(PercentValue value) {
    setAll(value);
}

PercentValue BorderRadius::getTopLeft() const {
    return getValue(_topLeft, _topLeftIsPercent);
}

PercentValue BorderRadius::getTopRight() const {
    return getValue(_topRight, _topRightIsPercent);
}

PercentValue BorderRadius::getBottomRight() const {
    return getValue(_bottomRight, _bottomRightIsPercent);
}

PercentValue BorderRadius::getBottomLeft() const {
    return getValue(_bottomLeft, _bottomLeftIsPercent);
}

void BorderRadius::setTopLeft(PercentValue topLeft) {
    setValue(topLeft, _topLeft, _topLeftIsPercent);
}

void BorderRadius::setTopRight(PercentValue topRight) {
    setValue(topRight, _topRight, _topRightIsPercent);
}

void BorderRadius::setBottomRight(PercentValue bottomRight) {
    setValue(bottomRight, _bottomRight, _bottomRightIsPercent);
}

void BorderRadius::setBottomLeft(PercentValue bottomLeft) {
    setValue(bottomLeft, _bottomLeft, _bottomLeftIsPercent);
}

void BorderRadius::setAll(PercentValue value) {
    setTopLeft(value);
    setTopRight(value);
    setBottomRight(value);
    setBottomLeft(value);
}

bool BorderRadius::areBordersEqual() const {
    return _topLeft == _topRight && _topRight == _bottomRight && _bottomRight == _bottomLeft &&
           _topLeftIsPercent == _topRightIsPercent && _topRightIsPercent == _bottomRightIsPercent &&
           _bottomRightIsPercent == _bottomLeftIsPercent;
}

bool BorderRadius::operator==(const BorderRadius& other) const {
    if (this == &other) {
        return true;
    }

    return _topLeft == other._topLeft && _topRight == other._topRight && _bottomRight == other._bottomRight &&
           _bottomLeft == other._bottomLeft && _topLeftIsPercent == other._topLeftIsPercent &&
           _topRightIsPercent == other._topRightIsPercent && _bottomRightIsPercent == other._bottomRightIsPercent &&
           _bottomLeftIsPercent == other._bottomLeftIsPercent;
}

bool BorderRadius::operator!=(const BorderRadius& other) const {
    return !(*this == other);
}

VALDI_CLASS_IMPL(BorderRadius)

} // namespace Valdi
