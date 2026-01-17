#include <gtest/gtest.h>

#include "DisplayListBuilder.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include <vector>

using namespace Valdi;

namespace snap::drawing {

static LayerContent makeRectangle(Size size) {
    DrawingContext ctx(size.width, size.height);

    Paint paint;
    paint.setColor(Color::blue());

    ctx.drawPaint(paint, ctx.drawBounds());

    return ctx.finish();
}

TEST(DisplayList, canRecordAndVisitCommands) {
    auto displayList = makeShared<DisplayList>(Size(100, 100), TimePoint(0));

    auto layerContent = makeRectangle(Size(50, 500));
    auto borderRadius = BorderRadius::makeOval(25, true);

    Matrix matrix;
    matrix.setTranslateX(42);
    matrix.setScaleY(2);

    displayList->pushContext(matrix, 0.5, 0, true);
    displayList->appendLayerContent(layerContent, 0.25);
    displayList->appendClipRound(borderRadius, 25, 30);
    displayList->appendClipRect(40, 70);
    displayList->popContext();

    ASSERT_EQ(static_cast<size_t>(1), displayList->getPlanesCount());

    auto operations = getOperationsFromDisplayList(displayList, 0);

    ASSERT_EQ(static_cast<size_t>(5), operations.size());

    ASSERT_EQ(Operations::PushContext::kId, operations[0]->type);
    ASSERT_EQ(Operations::DrawPicture::kId, operations[1]->type);
    ASSERT_EQ(Operations::ClipRound::kId, operations[2]->type);
    ASSERT_EQ(Operations::ClipRect::kId, operations[3]->type);
    ASSERT_EQ(Operations::PopContext::kId, operations[4]->type);

    const auto* pushContext = reinterpret_cast<const Operations::PushContext*>(operations[0]);
    const auto* drawPicture = reinterpret_cast<const Operations::DrawPicture*>(operations[1]);
    const auto* clipRound = reinterpret_cast<const Operations::ClipRound*>(operations[2]);
    const auto* clipRect = reinterpret_cast<const Operations::ClipRect*>(operations[3]);

    ASSERT_EQ(matrix, pushContext->matrix);
    ASSERT_EQ(0.5, pushContext->opacity);

    ASSERT_EQ(layerContent.picture.get(), drawPicture->picture);
    ASSERT_EQ(0.25, drawPicture->opacity);

    ASSERT_EQ(borderRadius, clipRound->borderRadius);
    ASSERT_EQ(25, clipRound->width);
    ASSERT_EQ(30, clipRound->height);

    ASSERT_EQ(40, clipRect->width);
    ASSERT_EQ(70, clipRect->height);
}

TEST(DisplayList, retainAndReleaseRetainableObjects) {
    auto displayList = makeShared<DisplayList>(Size(100, 100), TimePoint(0));

    auto rectanglePicture = makeRectangle(Size(50, 500)).picture;
    auto externalSurface = makeShared<ExternalSurface>();

    ASSERT_EQ(1, externalSurface.use_count());

    displayList->appendLayerContent(
        LayerContent(rectanglePicture, makeShared<ExternalSurfaceSnapshot>(externalSurface)), 1.0);

    ASSERT_EQ(2, externalSurface.use_count());

    // We can't explicitly check for ref count of SkPicture, but we can check that we can make calls to it
    // after we release our own ref
    auto* rectanglePicturePtr = rectanglePicture.get();

    ASSERT_EQ(Rect(0, 0, 50, 500), fromSkValue<Rect>(rectanglePicturePtr->cullRect()));
    rectanglePicture = nullptr;
    ASSERT_EQ(Rect(0, 0, 50, 500), fromSkValue<Rect>(rectanglePicturePtr->cullRect()));

    displayList = nullptr;

    ASSERT_EQ(1, externalSurface.use_count());
}

TEST(DisplayList, releaseRetainableObjectsWhenRemovingPlane) {
    auto displayList = makeShared<DisplayList>(Size(100, 100), TimePoint(0));

    displayList->appendPlane();
    displayList->appendPlane();

    displayList->setCurrentPlane(1);

    auto externalSurface = makeShared<ExternalSurface>();

    ASSERT_EQ(1, externalSurface.use_count());

    displayList->appendLayerContent(LayerContent(nullptr, makeShared<ExternalSurfaceSnapshot>(externalSurface)), 1.0);

    ASSERT_EQ(2, externalSurface.use_count());

    displayList->removePlane(2);

    ASSERT_EQ(2, externalSurface.use_count());

    displayList->removePlane(1);

    ASSERT_EQ(1, externalSurface.use_count());

    displayList->removePlane(0);

    ASSERT_EQ(1, externalSurface.use_count());
}

TEST(DisplayList, canHoldMultipleSurfaces) {
    auto displayList = makeShared<DisplayList>(Size(100, 100), TimePoint(0));

    auto layerContent = makeRectangle(Size(50, 500));
    auto borderRadius = BorderRadius::makeOval(25, true);

    Matrix matrix;
    matrix.setTranslateX(42);
    matrix.setScaleY(2);

    displayList->pushContext(matrix, 0.5, 0, true);

    displayList->appendPlane();
    displayList->appendLayerContent(layerContent, 0.25);

    displayList->appendPlane();
    displayList->appendClipRound(borderRadius, 25, 30);
    displayList->appendClipRect(40, 70);

    ASSERT_EQ(static_cast<size_t>(3), displayList->getPlanesCount());

    auto operations1 = getOperationsFromDisplayList(displayList, 0);
    auto operations2 = getOperationsFromDisplayList(displayList, 1);
    auto operations3 = getOperationsFromDisplayList(displayList, 2);

    ASSERT_EQ(static_cast<size_t>(1), operations1.size());
    ASSERT_EQ(static_cast<size_t>(1), operations2.size());
    ASSERT_EQ(static_cast<size_t>(2), operations3.size());

    ASSERT_EQ(Operations::PushContext::kId, operations1[0]->type);
    ASSERT_EQ(Operations::DrawPicture::kId, operations2[0]->type);
    ASSERT_EQ(Operations::ClipRound::kId, operations3[0]->type);
    ASSERT_EQ(Operations::ClipRect::kId, operations3[1]->type);

    const auto* pushContext = reinterpret_cast<const Operations::PushContext*>(operations1[0]);
    const auto* drawPicture = reinterpret_cast<const Operations::DrawPicture*>(operations2[0]);
    const auto* clipRound = reinterpret_cast<const Operations::ClipRound*>(operations3[0]);
    const auto* clipRect = reinterpret_cast<const Operations::ClipRect*>(operations3[1]);

    ASSERT_EQ(matrix, pushContext->matrix);
    ASSERT_EQ(0.5, pushContext->opacity);

    ASSERT_EQ(layerContent.picture.get(), drawPicture->picture);
    ASSERT_EQ(0.25, drawPicture->opacity);

    ASSERT_EQ(borderRadius, clipRound->borderRadius);
    ASSERT_EQ(25, clipRound->width);
    ASSERT_EQ(30, clipRound->height);

    ASSERT_EQ(40, clipRect->width);
    ASSERT_EQ(70, clipRect->height);

    // We should also be able to visit them all at once
    auto allOperations = getOperationsFromDisplayList(displayList, kDisplayListAllPlaneIndexes);

    ASSERT_EQ(static_cast<size_t>(4), allOperations.size());

    ASSERT_EQ(pushContext, allOperations[0]);
    ASSERT_EQ(drawPicture, allOperations[1]);
    ASSERT_EQ(clipRound, allOperations[2]);
    ASSERT_EQ(clipRect, allOperations[3]);
}

TEST(DisplayList, canRemoveEmptySurfaces) {
    auto displayList = makeShared<DisplayList>(Size(100, 100), TimePoint(0));

    Matrix matrix;
    matrix.setTranslateX(42);
    matrix.setScaleY(2);

    displayList->appendPlane();

    displayList->pushContext(matrix, 0.5, 0, true);

    displayList->appendPlane();

    displayList->appendPlane();
    displayList->appendClipRect(40, 70);

    displayList->appendPlane();

    ASSERT_EQ(static_cast<size_t>(5), displayList->getPlanesCount());

    displayList->removeEmptyPlanes();

    ASSERT_EQ(static_cast<size_t>(2), displayList->getPlanesCount());

    auto operations1 = getOperationsFromDisplayList(displayList, 0);
    auto operations2 = getOperationsFromDisplayList(displayList, 1);

    ASSERT_EQ(static_cast<size_t>(1), operations1.size());
    ASSERT_EQ(static_cast<size_t>(1), operations2.size());

    ASSERT_EQ(Operations::PushContext::kId, operations1[0]->type);
    ASSERT_EQ(Operations::ClipRect::kId, operations2[0]->type);

    const auto* pushContext = reinterpret_cast<const Operations::PushContext*>(operations1[0]);
    const auto* clipRect = reinterpret_cast<const Operations::ClipRect*>(operations2[0]);

    ASSERT_EQ(matrix, pushContext->matrix);
    ASSERT_EQ(0.5, pushContext->opacity);

    ASSERT_EQ(40, clipRect->width);
    ASSERT_EQ(70, clipRect->height);
}

} // namespace snap::drawing
