//
//  Matrix.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/SkiaBridge.hpp"

#include "include/core/SkMatrix.h"

#include <ostream>
#include <string>

namespace snap::drawing {

class Matrix : public WrappedSkValue<SkMatrix> {
public:
    void setIdentity();

    Scalar getTranslateX() const;
    void setTranslateX(Scalar translateX);

    Scalar getTranslateY() const;
    void setTranslateY(Scalar translateY);

    Scalar getScaleX() const;
    void setScaleX(Scalar scaleX);

    Scalar getScaleY() const;
    void setScaleY(Scalar scaleY);

    Scalar getSkewX() const;
    void setSkewX(Scalar skewX);

    Scalar getSkewY() const;
    void setSkewY(Scalar skewY);

    void postRotate(Scalar radians, Scalar px, Scalar py);
    void postRotateDegrees(Scalar degrees, Scalar px, Scalar py);

    void preConcat(const Matrix& other);
    void postConcat(const Matrix& other);

    void preScale(Scalar sx, Scalar sy);
    void postScale(Scalar sx, Scalar sy);

    Rect mapRect(const Rect& rect) const;

    /**
     Append all the Matrix values into the given array.
     The array must be of length 9.
     */
    void getAll(Scalar* values) const;

    std::string toString() const;

    bool isIdentity() const;

    bool isIdentityOrTranslate() const;

    /**
     Given array must be of length 6
     */
    bool toAffine(Scalar* affineValues) const;

    bool operator==(const Matrix& other) const;
    bool operator!=(const Matrix& other) const;

    static Matrix makeScaleTranslate(Scalar sx, Scalar sy, Scalar tx, Scalar ty);
    static Matrix makeScale(Scalar sx, Scalar sy);
};

std::ostream& operator<<(std::ostream& os, const Matrix& matrix);

} // namespace snap::drawing
