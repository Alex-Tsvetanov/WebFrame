#include <gtest/gtest.h>

#include "DisplayListBuilder.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Layers/Mask/PaintMaskLayer.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

using namespace Valdi;

namespace snap::drawing {

class LayerTests : public ::testing::Test {
protected:
    void SetUp() override {
        _resources = makeShared<Resources>(nullptr, 1.0f, ConsoleLogger::getLogger());

        _root = createLayer();
    }

    void TearDown() override {
        _resources = nullptr;
        _root = nullptr;
    }

    Ref<Layer> createLayer() {
        return makeLayer<Layer>(_resources);
    }

    DrawMetrics draw() {
        DrawMetrics metrics;

        if (_root->childNeedsDisplay()) {
            auto displayList = makeShared<DisplayList>(Size(), TimePoint::fromSeconds(0.0));

            _root->draw(*displayList, metrics);
        }

        return metrics;
    }

    Ref<Resources> _resources;
    Ref<Layer> _root;
};

TEST_F(LayerTests, visitsChildrenOnDraw) {
    auto metrics = draw();
    ASSERT_EQ(1, metrics.drawCacheMiss);
    ASSERT_EQ(1, metrics.visitedLayers);

    auto container = createLayer();
    auto child1 = createLayer();
    auto child2 = createLayer();

    _root->addChild(container);
    container->addChild(child1);
    container->addChild(child2);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_TRUE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_TRUE(child1->needsDisplay());
    ASSERT_TRUE(child1->childNeedsDisplay());
    ASSERT_TRUE(child2->needsDisplay());
    ASSERT_TRUE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(3, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(0, metrics.drawCacheMiss);
    ASSERT_EQ(0, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());
}

TEST_F(LayerTests, visitsChildAgainAfterReinsertion) {
    auto container = createLayer();
    auto child1 = createLayer();
    auto child2 = createLayer();

    _root->addChild(container);
    container->addChild(child1);
    container->addChild(child2);

    auto metrics = draw();
    ASSERT_EQ(4, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    child2->removeFromParent();

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(0, metrics.drawCacheMiss);
    ASSERT_EQ(3, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    container->addChild(child2);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_TRUE(child2->needsDisplay());
    ASSERT_TRUE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(1, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(0, metrics.drawCacheMiss);
    ASSERT_EQ(0, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());
}

TEST_F(LayerTests, propagatesNeedsDisplay) {
    auto container = createLayer();
    auto child1 = createLayer();
    auto child2 = createLayer();

    _root->addChild(container);
    container->addChild(child1);
    container->addChild(child2);

    auto metrics = draw();
    ASSERT_EQ(4, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    child2->setNeedsDisplay();

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_TRUE(child2->needsDisplay());
    ASSERT_TRUE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(1, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    container->setNeedsDisplay();

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_TRUE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(1, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());

    container->setNeedsDisplay();
    child2->setNeedsDisplay();

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_TRUE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_TRUE(child2->needsDisplay());
    ASSERT_TRUE(child2->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(2, metrics.drawCacheMiss);
    ASSERT_EQ(4, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());
    ASSERT_FALSE(child2->needsDisplay());
    ASSERT_FALSE(child2->childNeedsDisplay());
}

TEST_F(LayerTests, doesntVisitInvisibleChild) {
    auto container = createLayer();
    auto child1 = createLayer();

    _root->addChild(container);
    container->addChild(child1);

    auto metrics = draw();
    ASSERT_EQ(3, metrics.drawCacheMiss);
    ASSERT_EQ(3, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_FALSE(container->needsDisplay());
    ASSERT_FALSE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());

    ASSERT_TRUE(container->isVisible());
    container->setOpacity(0);
    ASSERT_FALSE(container->isVisible());

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_TRUE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(0, metrics.drawCacheMiss);
    ASSERT_EQ(1, metrics.visitedLayers);

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_FALSE(_root->childNeedsDisplay());
    ASSERT_TRUE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());

    container->setOpacity(0.5);
    ASSERT_TRUE(container->isVisible());

    ASSERT_FALSE(_root->needsDisplay());
    ASSERT_TRUE(_root->childNeedsDisplay());
    ASSERT_TRUE(container->needsDisplay());
    ASSERT_TRUE(container->childNeedsDisplay());
    ASSERT_FALSE(child1->needsDisplay());
    ASSERT_FALSE(child1->childNeedsDisplay());

    metrics = draw();
    ASSERT_EQ(1, metrics.drawCacheMiss);
    ASSERT_EQ(3, metrics.visitedLayers);
}

TEST_F(LayerTests, canDrawMask) {
    auto maskLayer = Valdi::makeShared<PaintMaskLayer>();
    maskLayer->setRect(Rect::makeXYWH(5, 5, 10, 10));

    _root->setMaskLayer(maskLayer);
    _root->setFrame(Rect::makeXYWH(0, 0, 20, 20));

    auto displayList = makeShared<DisplayList>(Size(), TimePoint::fromSeconds(0.0));

    DrawMetrics metrics;
    _root->draw(*displayList, metrics);

    auto operations = getOperationsFromDisplayList(displayList, 0);

    ASSERT_EQ(static_cast<size_t>(4), operations.size());

    ASSERT_EQ(Operations::PrepareMask::kId, operations[1]->type);
    ASSERT_EQ(Operations::ApplyMask::kId, operations[2]->type);

    auto* expectedMask = reinterpret_cast<const Operations::PrepareMask*>(operations[1])->mask;
    ASSERT_TRUE(expectedMask != nullptr);

    ASSERT_EQ(expectedMask, reinterpret_cast<const Operations::ApplyMask*>(operations[2])->mask);
    ASSERT_EQ(Rect::makeXYWH(5, 5, 10, 10), expectedMask->getBounds());
}

TEST_F(LayerTests, canDrawMaskBelowBackground) {
    auto maskLayer = Valdi::makeShared<PaintMaskLayer>();
    maskLayer->setRect(Rect::makeXYWH(5, 5, 10, 10));
    maskLayer->setPositioning(MaskLayerPositioning::BelowBackground);

    _root->setMaskLayer(maskLayer);
    _root->setBackgroundColor(Color::red());
    _root->setFrame(Rect::makeXYWH(0, 0, 20, 20));

    auto displayList = makeShared<DisplayList>(Size(), TimePoint::fromSeconds(0.0));

    DrawMetrics metrics;
    _root->draw(*displayList, metrics);

    auto operations = getOperationsFromDisplayList(displayList, 0);

    ASSERT_EQ(static_cast<size_t>(5), operations.size());

    ASSERT_EQ(Operations::PrepareMask::kId, operations[1]->type);
    ASSERT_EQ(Operations::DrawPicture::kId, operations[2]->type);
    ASSERT_EQ(Operations::ApplyMask::kId, operations[3]->type);
}

TEST_F(LayerTests, canDrawMaskAboveBackground) {
    auto maskLayer = Valdi::makeShared<PaintMaskLayer>();
    maskLayer->setRect(Rect::makeXYWH(5, 5, 10, 10));
    maskLayer->setPositioning(MaskLayerPositioning::AboveBackground);

    _root->setMaskLayer(maskLayer);
    _root->setBackgroundColor(Color::red());
    _root->setFrame(Rect::makeXYWH(0, 0, 20, 20));

    auto displayList = makeShared<DisplayList>(Size(), TimePoint::fromSeconds(0.0));

    DrawMetrics metrics;
    _root->draw(*displayList, metrics);

    auto operations = getOperationsFromDisplayList(displayList, 0);

    ASSERT_EQ(static_cast<size_t>(5), operations.size());

    ASSERT_EQ(Operations::DrawPicture::kId, operations[1]->type);
    ASSERT_EQ(Operations::PrepareMask::kId, operations[2]->type);
    ASSERT_EQ(Operations::ApplyMask::kId, operations[3]->type);
}

} // namespace snap::drawing
