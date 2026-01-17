//
//  CleanUpDisplayListVisitor.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/DisplayList/CleanUpDisplayListVisitor.hpp"
#include "include/core/SkPicture.h"

#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"

namespace snap::drawing {

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::PushContext& /*pushContext*/) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::PopContext& /*popContext*/) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::DrawPicture& drawPicture) {
    drawPicture.picture->unref();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::ClipRect& /*clipRect*/) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::ClipRound& /*clipRound*/) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::DrawExternalSurface& drawExternalSurface) {
    drawExternalSurface.externalSurfaceSnapshot->unsafeReleaseInner();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::PrepareMask& prepareMask) {
    prepareMask.mask->unsafeReleaseInner();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CleanUpDisplayListVisitor::visit(const Operations::ApplyMask& applyMask) {
    applyMask.mask->unsafeReleaseInner();
}

} // namespace snap::drawing
