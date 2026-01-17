//
//  DisplayListOperations.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Scalar.hpp"

class SkPicture;

namespace snap::drawing {

class ExternalSurfaceSnapshot;
class IMask;

namespace Operations {

struct Operation {
    size_t type;
};

struct PushContext : public Operation {
    constexpr static size_t kId = 1;

    Matrix matrix;
    Scalar opacity;
    uint64_t layerId;
    bool hasUpdates;
};

struct PopContext : public Operation {
    constexpr static size_t kId = 2;
};

struct DrawPicture : public Operation {
    constexpr static size_t kId = 3;

    SkPicture* picture;
    Scalar opacity;
};

struct ClipOperation : public Operation {
    Scalar width;
    Scalar height;
};

struct ClipRect : public ClipOperation {
    constexpr static size_t kId = 4;
};

struct ClipRound : public ClipOperation {
    constexpr static size_t kId = 5;

    BorderRadius borderRadius;
};

struct DrawExternalSurface : public Operation {
    constexpr static size_t kId = 6;

    ExternalSurfaceSnapshot* externalSurfaceSnapshot;
    Scalar opacity;
};

struct PrepareMask : public Operation {
    constexpr static size_t kId = 7;

    IMask* mask;
};

struct ApplyMask : public Operation {
    constexpr static size_t kId = 8;

    IMask* mask;
};

template<typename Visitor>
inline auto visitOperation(const Operations::Operation& operation, Visitor& visitor) {
    switch (operation.type) {
        case Operations::PushContext::kId:
            return visitor.visit(reinterpret_cast<const Operations::PushContext&>(operation));
        case Operations::PopContext::kId:
            return visitor.visit(reinterpret_cast<const Operations::PopContext&>(operation));
        case Operations::DrawPicture::kId:
            return visitor.visit(reinterpret_cast<const Operations::DrawPicture&>(operation));
        case Operations::ClipRect::kId:
            return visitor.visit(reinterpret_cast<const Operations::ClipRect&>(operation));
        case Operations::ClipRound::kId:
            return visitor.visit(reinterpret_cast<const Operations::ClipRound&>(operation));
        case Operations::DrawExternalSurface::kId:
            return visitor.visit(reinterpret_cast<const Operations::DrawExternalSurface&>(operation));
        case Operations::PrepareMask::kId:
            return visitor.visit(reinterpret_cast<const Operations::PrepareMask&>(operation));
        case Operations::ApplyMask::kId:
            return visitor.visit(reinterpret_cast<const Operations::ApplyMask&>(operation));
        default:
            std::abort();
            break;
    }
}

}; // namespace Operations

}; // namespace snap::drawing
