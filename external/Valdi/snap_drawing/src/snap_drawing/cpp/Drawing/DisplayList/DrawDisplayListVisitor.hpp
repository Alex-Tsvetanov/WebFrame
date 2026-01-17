//
//  DrawDisplayListVisitor.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayListOperations.hpp"

class SkCanvas;

namespace snap::drawing {

/**
 DisplayList visitor which draws into a canvas
 */
class DrawDisplayListVisitor {
public:
    DrawDisplayListVisitor(SkCanvas* canvas, Scalar scaleX, Scalar scaleY);

    void visit(const Operations::PushContext& pushContext);

    void visit(const Operations::PopContext& popContext);

    void visit(const Operations::DrawPicture& drawPicture);

    void visit(const Operations::ClipRect& clipRect);

    void visit(const Operations::ClipRound& clipRound);

    void visit(const Operations::DrawExternalSurface& drawExternalSurface);

    void visit(const Operations::PrepareMask& prepareMask);

    void visit(const Operations::ApplyMask& applyMask);

private:
    SkCanvas* _canvas;
    Scalar _scaleX;
    Scalar _scaleY;
    Rect _tempRect;
};

} // namespace snap::drawing
