//
//  GeometricPath.hpp
//  valdi
//
//  Created by Simon Corsin on 3/4/22.
//

#pragma once

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"

namespace Valdi {

enum GeometricPathComponentType {
    GeometricPathComponentTypeMove = 1,
    GeometricPathComponentTypeLine = 2,
    GeometricPathComponentTypeQuad = 3,
    GeometricPathComponentTypeCubic = 4,
    GeometricPathComponentTypeRoundRect = 5,
    GeometricPathComponentTypeArc = 6,
    GeometricPathComponentTypeClose = 7,
};

struct GeometricPathVisitor {
    virtual ~GeometricPathVisitor() = default;

    virtual void moveTo(double x, double y) = 0;
    virtual void lineTo(double x, double y) = 0;
    virtual void quadTo(double x, double y, double controlX, double controlY) = 0;
    virtual void cubicTo(
        double x, double y, double controlX1, double controlY1, double controlX2, double controlY2) = 0;
    virtual void roundRectTo(double x, double y, double width, double height, double radiusX, double radiusY) = 0;
    virtual void arcTo(
        double centerX, double centerY, double radiusX, double radiusY, double startAngle, double sweepAngle) = 0;
    virtual void close() = 0;
};

class GeometricPathIterator {
public:
    GeometricPathIterator(const double* begin, const double* end);

    bool hasNext();
    bool next();
    bool readExtent();

    GeometricPathComponentType getComponentType() const;

    constexpr double getValue(size_t index) const {
        return _values[index];
    }

private:
    const double* _it;
    const double* _end;
    GeometricPathComponentType _componentType = GeometricPathComponentTypeMove;
    double _values[6];

    bool readValues(size_t size);
};

struct GeometricPath {
    static GeometricPathIterator iterator(const Byte* data, size_t size);

    static bool visit(const Byte* data, size_t size, double width, double height, GeometricPathVisitor& visitor);
};

} // namespace Valdi
