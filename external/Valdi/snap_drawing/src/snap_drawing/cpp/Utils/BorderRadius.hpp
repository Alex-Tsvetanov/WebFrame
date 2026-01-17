//
//  BorderRadius.hpp
//  valdi-skia-app
//
//  Created by Simon Corsin on 6/28/20.
//

#pragma once

#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"

#include <ostream>
#include <string>

namespace snap::drawing {

class BorderRadius {
public:
    BorderRadius(Scalar topLeft,
                 Scalar topRight,
                 Scalar bottomRight,
                 Scalar bottomLeft,
                 bool topLeftIsPercent,
                 bool topRightIsPercent,
                 bool bottomRightIsPercent,
                 bool bottomLeftIsPercent);
    BorderRadius();

    constexpr Scalar topLeft() const {
        return _topLeft;
    }

    constexpr Scalar topRight() const {
        return _topRight;
    }

    constexpr Scalar bottomLeft() const {
        return _bottomLeft;
    }

    constexpr Scalar bottomRight() const {
        return _bottomRight;
    }

    constexpr bool topLeftIsPercent() const {
        return _topLeftIsPercent;
    }

    constexpr bool topRightIsPercent() const {
        return _topRightIsPercent;
    }

    constexpr bool bottomLeftIsPercent() const {
        return _bottomLeftIsPercent;
    }

    constexpr bool bottomRightIsPercent() const {
        return _bottomRightIsPercent;
    }

    bool isEmpty() const;

    void applyToPath(const Rect& bounds, Path& path) const;
    Path getPath(const Rect& bounds) const;

    bool operator==(const BorderRadius& other) const;
    bool operator!=(const BorderRadius& other) const;

    std::string toString() const;

    static Scalar sideLengthForPercentages(const Rect& bounds);
    static BorderRadius makeCircle();
    static BorderRadius makeOval(Scalar corners, bool isPercent);

private:
    Scalar _topLeft = 0;
    Scalar _topRight = 0;
    Scalar _bottomRight = 0;
    Scalar _bottomLeft = 0;

    bool _topLeftIsPercent = false;
    bool _topRightIsPercent = false;
    bool _bottomRightIsPercent = false;
    bool _bottomLeftIsPercent = false;
    bool _isEmpty = true;
};

std::ostream& operator<<(std::ostream& os, const BorderRadius& borderRadius);

} // namespace snap::drawing
