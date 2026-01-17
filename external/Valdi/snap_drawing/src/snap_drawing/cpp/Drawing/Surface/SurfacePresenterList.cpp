//
//  SurfacePresenterList.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/16/22.
//

#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterList.hpp"
#include <algorithm>

namespace snap::drawing {

SurfacePresenterList::SurfacePresenterList() = default;
SurfacePresenterList::~SurfacePresenterList() = default;

size_t SurfacePresenterList::size() const {
    return _surfacePresenters.size();
}

const SurfacePresenter* SurfacePresenterList::begin() const {
    return _surfacePresenters.data();
}

const SurfacePresenter* SurfacePresenterList::end() const {
    return _surfacePresenters.data() + _surfacePresenters.size();
}

SurfacePresenter* SurfacePresenterList::begin() {
    return _surfacePresenters.data();
}

SurfacePresenter* SurfacePresenterList::end() {
    return _surfacePresenters.data() + _surfacePresenters.size();
}

const SurfacePresenter& SurfacePresenterList::operator[](size_t i) const {
    return _surfacePresenters[i];
}

SurfacePresenter& SurfacePresenterList::operator[](size_t i) {
    return _surfacePresenters[i];
}

const SurfacePresenter* SurfacePresenterList::getForId(SurfacePresenterId id) const {
    // Could later also introduce a map if we want fast look up for the surface presenters.
    // In practice this call should be not frequent, and the number of presenters should be
    // very low, so this might not be necessary.
    for (const auto& surfacePresenter : _surfacePresenters) {
        if (surfacePresenter.getId() == id) {
            return &surfacePresenter;
        }
    }
    return nullptr;
}

SurfacePresenter* SurfacePresenterList::getForId(SurfacePresenterId id) {
    for (auto& surfacePresenter : _surfacePresenters) {
        if (surfacePresenter.getId() == id) {
            return &surfacePresenter;
        }
    }
    return nullptr;
}

SurfacePresenter& SurfacePresenterList::append(SurfacePresenterId id) {
    return insert(size(), id);
}

SurfacePresenter& SurfacePresenterList::insert(size_t index, SurfacePresenterId id) {
    return *_surfacePresenters.insert(_surfacePresenters.begin() + index, SurfacePresenter(id));
}

void SurfacePresenterList::move(size_t fromIndex, size_t toIndex) {
    if (fromIndex > toIndex) {
        std::rotate(_surfacePresenters.rend() - fromIndex - 1,
                    _surfacePresenters.rend() - fromIndex,
                    _surfacePresenters.rend() - toIndex);
    } else {
        std::rotate(_surfacePresenters.begin() + fromIndex,
                    _surfacePresenters.begin() + fromIndex + 1,
                    _surfacePresenters.begin() + toIndex + 1);
    }
}

void SurfacePresenterList::remove(size_t i) {
    _surfacePresenters.erase(_surfacePresenters.begin() + i);
}

} // namespace snap::drawing
