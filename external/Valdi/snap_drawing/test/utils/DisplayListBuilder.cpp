#include "DisplayListBuilder.hpp"
#include "snap_drawing/cpp/Drawing/Mask/PaintMask.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"

namespace snap::drawing {

DisplayListBuilder::DisplayListBuilder(Scalar width, Scalar height)
    : displayList(Valdi::makeShared<DisplayList>(Size(width, height), TimePoint(0.0))) {}

void DisplayListBuilder::context(Vector translation, Scalar opacity, BuilderCb&& cb) {
    context(translation, opacity, 0, true, std::move(cb));
}

void DisplayListBuilder::context(const Matrix& matrix, Scalar opacity, BuilderCb&& cb) {
    context(matrix, opacity, 0, true, std::move(cb));
}

void DisplayListBuilder::context(
    Vector translation, Scalar opacity, uint64_t layerId, bool hasUpdates, BuilderCb&& cb) {
    Matrix matrix;
    matrix.setTranslateX(translation.dx);
    matrix.setTranslateY(translation.dy);

    context(matrix, opacity, layerId, hasUpdates, std::move(cb));
}

void DisplayListBuilder::context(
    const Matrix& matrix, Scalar opacity, uint64_t layerId, bool hasUpdates, BuilderCb&& cb) {
    displayList->pushContext(matrix, opacity, layerId, hasUpdates);

    cb();

    displayList->popContext();
}

void DisplayListBuilder::layerContent(const LayerContent& layerContent, Scalar opacity) {
    displayList->appendLayerContent(layerContent, opacity);
}

LayerContent DisplayListBuilder::draw(Size size, Scalar opacity, Valdi::Function<void(DrawingContext&)>&& cb) {
    DrawingContext ctx(size.width, size.height);

    cb(ctx);

    auto content = ctx.finish();

    layerContent(content, opacity);

    return content;
}

LayerContent DisplayListBuilder::rectangle(Size size, Scalar opacity) {
    return draw(size, opacity, [](DrawingContext& drawingContext) {
        Paint paint;
        paint.setColor(Color::blue());

        drawingContext.drawPaint(paint, drawingContext.drawBounds());
    });
}

LayerContent DisplayListBuilder::externalSurface(Size size, Scalar opacity) {
    return draw(size, opacity, [&](DrawingContext& drawingContext) {
        auto externalSurface = Valdi::makeShared<ExternalSurface>();
        externalSurface->setRelativeSize(size);

        drawingContext.drawExternalSurface(externalSurface);
    });
}

void DisplayListBuilder::clip(Size size) {
    displayList->appendClipRect(size.width, size.height);
}

void DisplayListBuilder::clip(const BorderRadius& borderRadius, Size size) {
    displayList->appendClipRound(borderRadius, size.width, size.height);
}

void DisplayListBuilder::plane(BuilderCb&& cb) {
    displayList->appendPlane();
    cb();
}

void DisplayListBuilder::mask(const Rect& rect, BuilderCb&& cb) {
    Paint paint;
    auto mask = Valdi::makeShared<PaintMask>(paint, Path(), rect);
    displayList->appendPrepareMask(mask.get());
    cb();
    displayList->appendApplyMask(mask.get());
}

struct ToArrayVisitor {
    std::vector<const Operations::Operation*> operations;

    template<typename T>
    void visit(const T& operation) {
        operations.emplace_back(&static_cast<const Operations::Operation&>(operation));
    }
};

std::vector<const Operations::Operation*> getOperationsFromDisplayList(const DisplayList& displayList,
                                                                       size_t planeIndex) {
    ToArrayVisitor visitor;
    displayList.visitOperations(planeIndex, visitor);

    return std::move(visitor.operations);
}

std::vector<const Operations::Operation*> getOperationsFromDisplayList(const Ref<DisplayList>& displayList,
                                                                       size_t planeIndex) {
    return getOperationsFromDisplayList(*displayList, planeIndex);
}

bool operator==(const DisplayList& left, const DisplayList& right) {
    return left.toDebugJSON() == right.toDebugJSON();
}

bool operator!=(const DisplayList& left, const DisplayList& right) {
    return !(left == right);
}

std::ostream& operator<<(std::ostream& os, const DisplayList& displayList) {
    return os << displayList.toDebugJSON().toString(true);
}

} // namespace snap::drawing
