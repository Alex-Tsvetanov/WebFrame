//
//  BorderRadius.hpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 6/24/21.
//

#pragma once

#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

struct PercentValue {
    double value;
    bool isPercent;

    inline PercentValue(double value, bool isPercent) : value(value), isPercent(isPercent) {}

    constexpr bool operator==(const PercentValue& other) const {
        return value == other.value && isPercent == other.isPercent;
    }

    constexpr bool operator!=(const PercentValue& other) const {
        return !(*this == other);
    }
};

class BorderRadius : public ValdiObject {
public:
    BorderRadius();
    explicit BorderRadius(PercentValue value);

    VALDI_CLASS_HEADER(BorderRadius)

    PercentValue getTopLeft() const;
    PercentValue getTopRight() const;
    PercentValue getBottomRight() const;
    PercentValue getBottomLeft() const;

    void setTopLeft(PercentValue topLeft);
    void setTopRight(PercentValue topRight);
    void setBottomRight(PercentValue bottomRight);
    void setBottomLeft(PercentValue bottomLeft);

    void setAll(PercentValue value);

    constexpr double getTopLeftValue() const {
        return _topLeft;
    }

    constexpr double getTopRightValue() const {
        return _topRight;
    }

    constexpr double getBottomRightValue() const {
        return _bottomRight;
    }

    constexpr double getBottomLeftValue() const {
        return _bottomLeft;
    }

    constexpr bool isTopLeftPercent() const {
        return _topLeftIsPercent;
    }

    constexpr bool isTopRightPercent() const {
        return _topRightIsPercent;
    }

    constexpr bool isBottomRightPercent() const {
        return _bottomRightIsPercent;
    }

    constexpr bool isBottomLeftPercent() const {
        return _bottomLeftIsPercent;
    }

    /**
     Whether all borders are of the same value
     */
    bool areBordersEqual() const;

    bool operator==(const BorderRadius& other) const;
    bool operator!=(const BorderRadius& other) const;

private:
    double _topLeft = 0;
    double _topRight = 0;
    double _bottomRight = 0;
    double _bottomLeft = 0;
    bool _topLeftIsPercent = false;
    bool _topRightIsPercent = false;
    bool _bottomRightIsPercent = false;
    bool _bottomLeftIsPercent = false;
};

} // namespace Valdi
