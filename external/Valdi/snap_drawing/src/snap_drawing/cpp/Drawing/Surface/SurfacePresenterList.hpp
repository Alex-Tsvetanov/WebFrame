//
//  SurfacePresenterList.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/16/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenter.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include <optional>

namespace snap::drawing {

class SurfacePresenterList {
public:
    SurfacePresenterList();
    ~SurfacePresenterList();

    size_t size() const;

    const SurfacePresenter* begin() const;
    const SurfacePresenter* end() const;
    SurfacePresenter* begin();
    SurfacePresenter* end();

    const SurfacePresenter& operator[](size_t i) const;
    SurfacePresenter& operator[](size_t i);

    const SurfacePresenter* getForId(SurfacePresenterId id) const;
    SurfacePresenter* getForId(SurfacePresenterId id);

    SurfacePresenter& insert(size_t index, SurfacePresenterId id);

    SurfacePresenter& append(SurfacePresenterId id);

    void move(size_t fromIndex, size_t toIndex);

    void remove(size_t i);

private:
    Valdi::SmallVector<SurfacePresenter, 2> _surfacePresenters;
};

} // namespace snap::drawing
