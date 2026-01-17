#pragma once

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include <vector>

namespace snap::drawing {

using BuilderCb = Valdi::Function<void()>;

struct DisplayListBuilder {
    Ref<DisplayList> displayList;

    DisplayListBuilder(Scalar width, Scalar height);

    void context(Vector translation, Scalar opacity, BuilderCb&& cb);

    void context(const Matrix& matrix, Scalar opacity, BuilderCb&& cb);

    void context(Vector translation, Scalar opacity, uint64_t layerId, bool hasUpdates, BuilderCb&& cb);

    void context(const Matrix& matrix, Scalar opacity, uint64_t layerId, bool hasUpdates, BuilderCb&& cb);

    LayerContent draw(Size size, Scalar opacity, Valdi::Function<void(DrawingContext&)>&& cb);

    LayerContent rectangle(Size size, Scalar opacity);

    LayerContent externalSurface(Size size, Scalar opacity);

    void mask(const Rect& rect, BuilderCb&& cb);

    void layerContent(const LayerContent& layerContent, Scalar opacity);

    void clip(Size size);
    void clip(const BorderRadius& borderRadius, Size size);

    void plane(BuilderCb&& cb);
};

std::vector<const Operations::Operation*> getOperationsFromDisplayList(const Ref<DisplayList>& displayList,
                                                                       size_t planeIndex);

bool operator==(const DisplayList& left, const DisplayList& right);
bool operator!=(const DisplayList& left, const DisplayList& right);
std::ostream& operator<<(std::ostream& os, const DisplayList& displayList);

} // namespace snap::drawing
