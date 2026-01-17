//
//  CleanUpDisplayListVisitor.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayListOperations.hpp"

namespace snap::drawing {

/**
 DisplayList visitor which cleans up the operations by releasing any
 retained refs.
 */
class CleanUpDisplayListVisitor {
public:
    void visit(const Operations::PushContext& pushContext);

    void visit(const Operations::PopContext& popContext);

    void visit(const Operations::DrawPicture& drawPicture);

    void visit(const Operations::ClipRect& clipRect);

    void visit(const Operations::ClipRound& clipRound);

    void visit(const Operations::DrawExternalSurface& drawExternalSurface);

    void visit(const Operations::PrepareMask& prepareMask);

    void visit(const Operations::ApplyMask& applyMask);
};

} // namespace snap::drawing
