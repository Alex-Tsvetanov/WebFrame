//
//  LazyPath.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/21/20.
//

#pragma once

#include "snap_drawing/cpp/Utils/Path.hpp"

namespace snap::drawing {

/**
 LazyPath offers a simple caching mecanism to avoid
 recreating Path on every draw. It assumes that the
 Path will only need be to be recreated if the size changes.
 */
class LazyPath {
public:
    LazyPath();
    ~LazyPath();

    constexpr bool update(Scalar width, Scalar height) {
        return update(Size::make(width, height));
    }

    void setNeedsUpdate();

    bool update(const Size& size);

    constexpr Path& path() {
        return _path;
    }

private:
    Path _path;
    Size _size = Size::makeEmpty();
};

} // namespace snap::drawing
