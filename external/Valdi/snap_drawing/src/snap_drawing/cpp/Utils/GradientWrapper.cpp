#include "snap_drawing/cpp/Utils/GradientWrapper.hpp"

namespace snap::drawing {

GradientWrapper::GradientWrapper() = default;

void GradientWrapper::update(const Rect& bounds) {
    if (_linearGradient != nullptr) {
        _linearGradient->update(bounds);
    } else if (_radialGradient != nullptr) {
        _radialGradient->update(bounds);
    }
}

void GradientWrapper::applyToPaint(Paint& paint) const {
    if (_linearGradient != nullptr) {
        _linearGradient->applyToPaint(paint);
    } else if (_radialGradient != nullptr) {
        _radialGradient->applyToPaint(paint);
    }
}

void GradientWrapper::draw(DrawingContext& drawingContext, const BorderRadius& borderRadius) {
    if (_linearGradient != nullptr) {
        _linearGradient->draw(drawingContext, borderRadius);
    } else if (_radialGradient != nullptr) {
        _radialGradient->draw(drawingContext, borderRadius);
    }
}

void GradientWrapper::setAsLinear(std::vector<Scalar>&& locations,
                                  std::vector<Color>&& colors,
                                  LinearGradientOrientation orientation) {
    if (_radialGradient != nullptr) {
        _radialGradient = nullptr;
    }

    if (_linearGradient == nullptr) {
        _linearGradient = Valdi::makeShared<LinearGradient>();
    }

    _linearGradient->setLocations(std::move(locations));
    _linearGradient->setColors(std::move(colors));
    _linearGradient->setOrientation(orientation);
}

Ref<LinearGradient> GradientWrapper::getLinearGradient() {
    return _linearGradient;
}

void GradientWrapper::setAsRadial(std::vector<Scalar>&& locations, std::vector<Color>&& colors) {
    if (_linearGradient != nullptr) {
        _linearGradient = nullptr;
    }

    if (_radialGradient == nullptr) {
        _radialGradient = Valdi::makeShared<RadialGradient>();
    }

    _radialGradient->setLocations(std::move(locations));
    _radialGradient->setColors(std::move(colors));
}

Ref<RadialGradient> GradientWrapper::getRadialGradient() {
    return _radialGradient;
}

void GradientWrapper::clear() {
    _linearGradient = nullptr;
    _radialGradient = nullptr;
}

// Returns a boolean value indicating whether there was an existing gradient
// of the given type that was cleared.
bool GradientWrapper::clearIfNeeded(GradientType type) {
    if (type == GradientType::LINEAR && _linearGradient != nullptr) {
        _linearGradient = nullptr;
        return true;
    } else if (type == GradientType::RADIAL && _radialGradient != nullptr) {
        _radialGradient = nullptr;
        return true;
    }

    return false;
}

bool GradientWrapper::isDirty() {
    if (_linearGradient != nullptr) {
        return _linearGradient->isDirty();
    } else if (_radialGradient != nullptr) {
        return _radialGradient->isDirty();
    }

    return false;
}

bool GradientWrapper::hasGradient() {
    return _linearGradient != nullptr || _radialGradient != nullptr;
}

} // namespace snap::drawing