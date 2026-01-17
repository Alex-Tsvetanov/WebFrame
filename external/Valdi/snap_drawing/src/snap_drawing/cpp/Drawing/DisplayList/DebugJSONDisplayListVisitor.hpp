//
//  DebugJSONDisplayListVisitor.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayListOperations.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include <vector>

namespace snap::drawing {

class DebugJSONDisplayListVisitor {
public:
    explicit DebugJSONDisplayListVisitor(std::vector<Valdi::Value>& output);

    void visit(const Operations::PushContext& pushContext);

    void visit(const Operations::PopContext& popContext);

    void visit(const Operations::DrawPicture& drawPicture);

    void visit(const Operations::ClipRect& clipRect);

    void visit(const Operations::ClipRound& clipRound);

    void visit(const Operations::DrawExternalSurface& drawExternalSurface);

    void visit(const Operations::PrepareMask& prepareMask);

    void visit(const Operations::ApplyMask& applyMask);

private:
    std::vector<Valdi::Value>& _output;

    Valdi::Value& append(std::string_view type);
};

} // namespace snap::drawing
