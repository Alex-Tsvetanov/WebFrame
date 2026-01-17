//
//  GeometricPath.cpp
//  valdi
//
//  Created by Simon Corsin on 3/4/22.
//

#include "valdi_core/cpp/Utils/GeometricPath.hpp"

namespace Valdi {

enum class GeometricPathScaleType {
    FILL = 1,
    CONTAIN = 2,
    COVER = 3,
    NONE = 4,
};

GeometricPathIterator::GeometricPathIterator(const double* begin, const double* end) : _it(begin), _end(end) {}

bool GeometricPathIterator::readValues(size_t size) {
    if (_it + size > _end) {
        return false;
    }

    for (size_t i = 0; i < size; i++) {
        _values[i] = *_it;
        _it++;
    }

    return true;
}

bool GeometricPathIterator::hasNext() {
    return _it != _end;
}

GeometricPathComponentType GeometricPathIterator::getComponentType() const {
    return _componentType;
}

bool GeometricPathIterator::readExtent() {
    return readValues(3);
}

bool GeometricPathIterator::next() {
    auto type = *_it;
    _it++;
    _componentType = static_cast<GeometricPathComponentType>(type);

    switch (_componentType) {
        case GeometricPathComponentTypeMove:
            return readValues(2);
        case GeometricPathComponentTypeLine:
            return readValues(2);
        case GeometricPathComponentTypeQuad:
            return readValues(4);
        case GeometricPathComponentTypeCubic:
        case GeometricPathComponentTypeRoundRect:
            return readValues(6);
        case GeometricPathComponentTypeArc:
            return readValues(5);
        case GeometricPathComponentTypeClose:
            return true;
        default:
            return false;
    }
}

GeometricPathIterator GeometricPath::iterator(const Byte* data, size_t size) {
    const auto* begin = reinterpret_cast<const double*>(data);
    const auto* end = begin + (size / sizeof(double));

    return GeometricPathIterator(begin, end);
}

bool GeometricPath::visit(const Byte* data, size_t size, double width, double height, GeometricPathVisitor& visitor) {
    auto it = GeometricPath::iterator(data, size);

    if (!it.readExtent()) {
        return false;
    }

    auto extentWidth = it.getValue(0);
    auto extentHeight = it.getValue(1);

    if (extentWidth <= 0 || extentHeight <= 0) {
        return false;
    }

    double xRatio = width / extentWidth;
    double yRatio = height / extentHeight;

    switch (static_cast<GeometricPathScaleType>(it.getValue(2))) {
        case GeometricPathScaleType::FILL:
            // Default behavior
            break;
        case GeometricPathScaleType::CONTAIN: {
            auto minRatio = std::min(xRatio, yRatio);
            xRatio = minRatio;
            yRatio = minRatio;
        } break;
        case GeometricPathScaleType::COVER: {
            auto maxRatio = std::max(xRatio, yRatio);
            xRatio = maxRatio;
            yRatio = maxRatio;
        } break;
        case GeometricPathScaleType::NONE:
            xRatio = 1.0;
            yRatio = 1.0;
            break;
        default:
            // Invalid scale type
            return false;
    }

    auto xOffset = (width - (extentWidth * xRatio)) / 2.0;
    auto yOffset = (height - (extentHeight * yRatio)) / 2.0;

    while (it.hasNext()) {
        if (!it.next()) {
            return false;
        }

        switch (it.getComponentType()) {
            case GeometricPathComponentTypeMove:
                visitor.moveTo(it.getValue(0) * xRatio + xOffset, it.getValue(1) * yRatio + yOffset);
                break;
            case GeometricPathComponentTypeLine:
                visitor.lineTo(it.getValue(0) * xRatio + xOffset, it.getValue(1) * yRatio + yOffset);
                break;
            case GeometricPathComponentTypeQuad:
                visitor.quadTo(it.getValue(0) * xRatio + xOffset,
                               it.getValue(1) * yRatio + yOffset,
                               it.getValue(2) * xRatio + xOffset,
                               it.getValue(3) * yRatio + yOffset);
                break;
            case GeometricPathComponentTypeCubic:
                visitor.cubicTo(it.getValue(0) * xRatio + xOffset,
                                it.getValue(1) * yRatio + yOffset,
                                it.getValue(2) * xRatio + xOffset,
                                it.getValue(3) * yRatio + yOffset,
                                it.getValue(4) * xRatio + xOffset,
                                it.getValue(5) * yRatio + yOffset);
                break;
            case GeometricPathComponentTypeRoundRect:
                visitor.roundRectTo(it.getValue(0) * xRatio + xOffset,
                                    it.getValue(1) * yRatio + yOffset,
                                    it.getValue(2) * xRatio,
                                    it.getValue(3) * yRatio,
                                    it.getValue(4) * xRatio,
                                    it.getValue(5) * yRatio);
                break;
            case GeometricPathComponentTypeArc:
                visitor.arcTo(it.getValue(0) * xRatio + xOffset,
                              it.getValue(1) * yRatio + yOffset,
                              it.getValue(2) * xRatio,
                              it.getValue(2) * yRatio,
                              it.getValue(3),
                              it.getValue(4));
                break;
            case GeometricPathComponentTypeClose:
                visitor.close();
                break;
        }
    }

    return true;
}

} // namespace Valdi
