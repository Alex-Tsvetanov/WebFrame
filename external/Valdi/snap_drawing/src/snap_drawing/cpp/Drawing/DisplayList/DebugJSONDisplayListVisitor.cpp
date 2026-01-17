//
//  DebugJSONDisplayListVisitor.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/DisplayList/DebugJSONDisplayListVisitor.hpp"
#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"

namespace snap::drawing {

DebugJSONDisplayListVisitor::DebugJSONDisplayListVisitor(std::vector<Valdi::Value>& output) : _output(output) {}

void DebugJSONDisplayListVisitor::visit(const Operations::PushContext& pushContext) {
    auto& op = append("push");
    op.setMapValue("matrix", Valdi::Value(pushContext.matrix.toString()));
    op.setMapValue("opacity", Valdi::Value(pushContext.opacity));
}

void DebugJSONDisplayListVisitor::visit(const Operations::PopContext& /*popContext*/) {
    append("pop");
}

void DebugJSONDisplayListVisitor::visit(const Operations::DrawPicture& drawPicture) {
    auto& op = append("draw");
    op.setMapValue("opacity", Valdi::Value(drawPicture.opacity));
    op.setMapValue("picturePtr", Valdi::Value(reinterpret_cast<int64_t>(drawPicture.picture)));
}

void DebugJSONDisplayListVisitor::visit(const Operations::ClipRect& clipRect) {
    auto& op = append("clipRect");
    op.setMapValue("width", Valdi::Value(clipRect.width));
    op.setMapValue("height", Valdi::Value(clipRect.height));
}

void DebugJSONDisplayListVisitor::visit(const Operations::ClipRound& clipRound) {
    auto& op = append("clipRound");
    op.setMapValue("width", Valdi::Value(clipRound.width));
    op.setMapValue("height", Valdi::Value(clipRound.height));
    op.setMapValue("borderRadius", Valdi::Value(clipRound.borderRadius.toString()));
}

void DebugJSONDisplayListVisitor::visit(const Operations::DrawExternalSurface& drawExternalSurface) {
    auto& op = append("clipRound");
    op.setMapValue("opacity", Valdi::Value(drawExternalSurface.opacity));
    op.setMapValue("externalSurfaceSnapshotPtr",
                   Valdi::Value(reinterpret_cast<int64_t>(drawExternalSurface.externalSurfaceSnapshot)));
}

void DebugJSONDisplayListVisitor::visit(const Operations::PrepareMask& prepareMask) {
    auto& op = append("prepareMask");
    op.setMapValue("description", Valdi::Value(prepareMask.mask->getDescription()));
}

void DebugJSONDisplayListVisitor::visit(const Operations::ApplyMask& applyMask) {
    auto& op = append("applyMask");
    op.setMapValue("description", Valdi::Value(applyMask.mask->getDescription()));
}

Valdi::Value& DebugJSONDisplayListVisitor::append(std::string_view type) {
    auto& value = _output.emplace_back();
    value.setMapValue("type", Valdi::Value(std::move(type)));
    return value;
}

} // namespace snap::drawing
