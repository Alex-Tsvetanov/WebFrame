//
//  LazyPath.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/21/20.
//

#include "snap_drawing/cpp/Utils/LazyPath.hpp"

namespace snap::drawing {

LazyPath::LazyPath() = default;

LazyPath::~LazyPath() = default;

void LazyPath::setNeedsUpdate() {
    _size = Size();
}

bool LazyPath::update(const Size& size) {
    if (size == _size) {
        return false;
    }
    _size = size;
    _path = Path();
    return true;
}

} // namespace snap::drawing
