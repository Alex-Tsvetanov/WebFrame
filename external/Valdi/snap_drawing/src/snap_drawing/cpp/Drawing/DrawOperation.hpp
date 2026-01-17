//
//  DrawOperation.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 2/16/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterList.hpp"

namespace snap::drawing {

class SurfacePresenterManager;

class DrawOperation : public Valdi::SimpleRefCountable {
public:
    DrawOperation(const Ref<DisplayList>& displayList,
                  const Ref<SurfacePresenterManager>& presenterManager,
                  SurfacePresenterList&& surfacePresenters);
    ~DrawOperation() override;

    bool drawForPresenterId(SurfacePresenterId presenterId, DrawableSurfaceCanvas& canvas);

    Valdi::Result<snap::drawing::GraphicsContext*> drawNext();
    bool hasNext();

private:
    Ref<DisplayList> _displayList;
    Ref<SurfacePresenterManager> _surfacePresenterManager;
    SurfacePresenterList _surfacePresenters;
    const SurfacePresenter* _current;

    void advance();
};

} // namespace snap::drawing
