//
//  DisplayList.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 9/15/20.
//

#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/CleanUpDisplayListVisitor.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/DebugJSONDisplayListVisitor.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/DrawDisplayListVisitor.hpp"
#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Paint.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurfaceCanvas.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkPicture.h"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include <cstdint>

namespace snap::drawing {

static void cleanUpByteBuffer(Valdi::ByteBuffer& byteBuffer) {
    byteBuffer.clear();
}

DisplayListPlane::DisplayListPlane(PooledByteBuffer&& operations) : operations(std::move(operations)) {}

size_t kDisplayListAllPlaneIndexes = std::numeric_limits<size_t>::max();

DisplayList::DisplayList(Size size, TimePoint frameTime) : _size(size), _frameTime(frameTime) {
    appendPlane();
}

DisplayList::~DisplayList() {
    removeAllPlanes();
}

Size DisplayList::getSize() const {
    return _size;
}

TimePoint DisplayList::getFrameTime() const {
    return _frameTime;
}

void DisplayList::pushContext(const Matrix& matrix, Scalar opacity, uint64_t layerId, bool hasUpdates) {
    auto* op = appendOperation<Operations::PushContext>();
    op->matrix = matrix;
    op->opacity = opacity;
    op->layerId = layerId;
    op->hasUpdates = hasUpdates;
}

void DisplayList::popContext() {
    appendOperation<Operations::PopContext>();
}

void DisplayList::appendLayerContent(const LayerContent& layerContent, Scalar opacity) {
    if (layerContent.picture != nullptr) {
        appendPicture(layerContent.picture.get(), opacity);
    }

    if (layerContent.externalSurface != nullptr) {
        auto* op = appendOperation<Operations::DrawExternalSurface>();
        op->externalSurfaceSnapshot = layerContent.externalSurface.get();
        op->opacity = opacity;

        op->externalSurfaceSnapshot->unsafeRetainInner();
        _hasExternalSurfaces = true;
    }
}

void DisplayList::appendPicture(SkPicture* picture, Scalar opacity) {
    auto* op = appendOperation<Operations::DrawPicture>();
    op->picture = picture;
    op->opacity = opacity;

    op->picture->ref();
}

void DisplayList::appendClipRound(const BorderRadius& borderRadius, Scalar width, Scalar height) {
    if (borderRadius.isEmpty()) {
        appendClipRect(width, height);
    } else {
        auto* op = appendOperation<Operations::ClipRound>();
        op->width = width;
        op->height = height;
        op->borderRadius = borderRadius;
    }
}

void DisplayList::appendClipRect(Scalar width, Scalar height) {
    auto* op = appendOperation<Operations::ClipRect>();
    op->width = width;
    op->height = height;
}

void DisplayList::appendPrepareMask(IMask* mask) {
    auto* op = appendOperation<Operations::PrepareMask>();
    op->mask = mask;
    op->mask->unsafeRetainInner();

    _hasMask = true;
}

void DisplayList::appendApplyMask(IMask* mask) {
    auto* op = appendOperation<Operations::ApplyMask>();
    op->mask = mask;
    op->mask->unsafeRetainInner();
}

size_t DisplayList::getBytesUsed(size_t planeIndex) const {
    auto ptrs = getBeginEndPtrs(planeIndex);

    return ptrs.second - ptrs.first;
}

size_t DisplayList::getPlanesCount() const {
    return _planes.size();
}

void DisplayList::appendPlane() {
    _currentPlane = &_planes.emplace_back(Valdi::ObjectPool<Valdi::ByteBuffer>::get().getOrCreate(&cleanUpByteBuffer));
}

void DisplayList::setCurrentPlane(size_t planeIndex) {
    _currentPlane = &_planes[planeIndex];
}

void DisplayList::removePlane(size_t planeIndex) {
    if (&_planes[planeIndex] == _currentPlane) {
        _currentPlane = nullptr;
    }

    CleanUpDisplayListVisitor visitor;
    visitOperations(planeIndex, visitor);

    _planes.erase(_planes.begin() + planeIndex);
}

void DisplayList::removeEmptyPlanes() {
    auto index = getPlanesCount();

    while (index > 0) {
        index--;

        if (getBytesUsed(index) == 0) {
            removePlane(index);
        }
    }
}

void DisplayList::removeAllPlanes() {
    auto size = getPlanesCount();
    while (size > 0) {
        auto planeIndex = --size;
        removePlane(planeIndex);
    }
}

bool DisplayList::hasExternalSurfaces() const {
    return _hasExternalSurfaces;
}

Valdi::Value DisplayList::toDebugJSON() const {
    Valdi::Value json;
    json.setMapValue("frameTime", Valdi::Value(_frameTime.getTime()));
    json.setMapValue("width", Valdi::Value(_size.width));
    json.setMapValue("height", Valdi::Value(_size.height));

    std::vector<Valdi::Value> surfaces;
    for (size_t i = 0; i < getPlanesCount(); i++) {
        std::vector<Valdi::Value> jsonOperations;
        DebugJSONDisplayListVisitor visitor(jsonOperations);

        visitOperations(i, visitor);

        surfaces.emplace_back(Valdi::ValueArray::make(std::move(jsonOperations)));
    }

    json.setMapValue("surfaces", Valdi::Value(Valdi::ValueArray::make(std::move(surfaces))));

    return json;
}

std::pair<Valdi::Byte*, Valdi::Byte*> DisplayList::getBeginEndPtrs(size_t planeIndex) const {
    const auto& operations = *_planes[planeIndex].operations;
    auto* beginPtr = operations.data();
    auto* endPtr = beginPtr + operations.size();

    return std::make_pair(beginPtr, endPtr);
}

static void prepareCanvas(SkCanvas* canvas, Scalar scaleX, Scalar scaleY, bool shouldClearCanvas) {
    canvas->scale(scaleX, scaleY);

    if (shouldClearCanvas) {
        Paint paint;
        paint.setColor(Color::transparent());
        paint.setBlendMode(SkBlendMode::kSrc);
        canvas->drawPaint(paint.getSkValue());
    }
}

void DisplayList::draw(
    DrawableSurfaceCanvas& canvas, size_t planeIndex, Scalar scaleX, Scalar scaleY, bool shouldClearCanvas) const {
    auto* skiaCanvas = canvas.getSkiaCanvas();

    auto saveCount = skiaCanvas->save();

    prepareCanvas(skiaCanvas, scaleX, scaleY, shouldClearCanvas);

    if (_hasMask) {
        // We need a dedicated layer texture to implement masking
        skiaCanvas->saveLayer(nullptr, nullptr);
    }

    DrawDisplayListVisitor visitor(skiaCanvas, scaleX, scaleY);
    visitOperations(planeIndex, visitor);

    skiaCanvas->restoreToCount(saveCount);
}

void DisplayList::draw(DrawableSurfaceCanvas& canvas, size_t planeIndex, bool shouldClearCanvas) const {
    auto canvasWidth = canvas.getWidth();
    auto canvasHeight = canvas.getHeight();

    auto scaleX = static_cast<Scalar>(canvasWidth) / _size.width;
    auto scaleY = static_cast<Scalar>(canvasHeight) / _size.height;

    draw(canvas, planeIndex, scaleX, scaleY, shouldClearCanvas);
}

} // namespace snap::drawing
