#include <gtest/gtest.h>

#include "snap_drawing/cpp/Layers/FlexboxLayer.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

using namespace Valdi;

namespace snap::drawing {

class FlexboxLayerTests : public ::testing::Test {
protected:
    void SetUp() override {
        _resources = makeShared<Resources>(nullptr, 1.0f, ConsoleLogger::getLogger());

        _layer = makeLayer<FlexboxLayer>(_resources);
    }

    void TearDown() override {
        _layer = nullptr;
    }

    Ref<Resources> _resources;
    Ref<FlexboxLayer> _layer;
};

TEST_F(FlexboxLayerTests, canLayoutNonFlexLayers) {
    auto child1 = makeLayer<Layer>(_resources);
    auto child2 = makeLayer<Layer>(_resources);
    auto child3 = makeLayer<Layer>(_resources);

    _layer->addChild(child1);
    _layer->addChild(child2);
    _layer->addChild(child3);

    _layer->updateLayoutAttributes()
        .setFlexDirection(YGFlexDirectionRow)
        .setJustifyContent(YGJustifyCenter)
        .setAlignItems(YGAlignCenter);

    _layer->updateLayoutAttributesForLayer(child1)
        .setWidth(FlexValue::point(16))
        .setHeight(FlexValue::point(16))
        .setMargin(YGEdgeRight, FlexValue::point(8));

    _layer->updateLayoutAttributesForLayer(child2).setWidth(FlexValue::point(10)).setHeight(FlexValue::point(10));

    _layer->updateLayoutAttributesForLayer(child3)
        .setWidth(FlexValue::point(8))
        .setHeight(FlexValue::point(8))
        .setMargin(YGEdgeLeft, FlexValue::point(8));

    _layer->setFrame(Rect::makeXYWH(0, 0, 50, 16));
    _layer->layoutIfNeeded();

    ASSERT_EQ(Rect::makeXYWH(0, 0, 16, 16), child1->getFrame());
    ASSERT_EQ(Rect::makeXYWH(24, 3, 10, 10), child2->getFrame());
    ASSERT_EQ(Rect::makeXYWH(42, 4, 8, 8), child3->getFrame());

    _layer->updateLayoutAttributesForLayer(child1).setMargin(YGEdgeRight, FlexValue::point(4));

    _layer->updateLayoutAttributesForLayer(child3)
        .setMargin(YGEdgeLeft, FlexValue::point(6))
        .setMargin(YGEdgeRight, FlexValue::point(6));

    ASSERT_EQ(Rect::makeXYWH(0, 0, 16, 16), child1->getFrame());
    ASSERT_EQ(Rect::makeXYWH(24, 3, 10, 10), child2->getFrame());
    ASSERT_EQ(Rect::makeXYWH(42, 4, 8, 8), child3->getFrame());

    _layer->layoutIfNeeded();

    ASSERT_EQ(Rect::makeXYWH(0, 0, 16, 16), child1->getFrame());
    ASSERT_EQ(Rect::makeXYWH(20, 3, 10, 10), child2->getFrame());
    ASSERT_EQ(Rect::makeXYWH(36, 4, 8, 8), child3->getFrame());
}

TEST_F(FlexboxLayerTests, canLayoutFlexLayers) {
    auto child1 = makeLayer<Layer>(_resources);
    auto childLayout = makeLayer<FlexboxLayer>(_resources);

    _layer->addChild(child1);
    _layer->addChild(childLayout);

    auto innerChild = makeLayer<Layer>(_resources);

    childLayout->addChild(innerChild);

    _layer->updateLayoutAttributes()
        .setFlexDirection(YGFlexDirectionRow)
        .setJustifyContent(YGJustifyCenter)
        .setAlignItems(YGAlignCenter);

    _layer->updateLayoutAttributesForLayer(child1)
        .setWidth(FlexValue::point(10))
        .setHeight(FlexValue::point(10))
        .setMargin(YGEdgeRight, FlexValue::point(8));

    _layer->updateLayoutAttributesForLayer(childLayout).setPadding(YGEdgeAll, FlexValue::point(4));

    _layer->updateLayoutAttributesForLayer(innerChild).setWidth(FlexValue::point(8)).setHeight(FlexValue::point(8));

    _layer->setFrame(Rect::makeXYWH(0, 0, 34, 16));
    _layer->layoutIfNeeded();

    ASSERT_EQ(Rect::makeXYWH(0, 3, 10, 10), child1->getFrame());
    ASSERT_EQ(Rect::makeXYWH(18, 0, 16, 16), childLayout->getFrame());
    ASSERT_EQ(Rect::makeXYWH(4, 4, 8, 8), innerChild->getFrame());

    _layer->updateLayoutAttributesForLayer(innerChild).setWidth(FlexValue::point(1));

    _layer->layoutIfNeeded();

    ASSERT_EQ(Rect::makeXYWH(3, 3, 10, 10), child1->getFrame());
    ASSERT_EQ(Rect::makeXYWH(22, 0, 9, 16), childLayout->getFrame());
    ASSERT_EQ(Rect::makeXYWH(4, 4, 1, 8), innerChild->getFrame());
}

TEST_F(FlexboxLayerTests, implementsSizeThatFits) {
    auto child1 = makeLayer<Layer>(_resources);

    _layer->addChild(child1);

    _layer->updateLayoutAttributes().setFlexDirection(YGFlexDirectionRow);
    ;

    _layer->updateLayoutAttributesForLayer(child1).setWidth(FlexValue::point(16)).setHeight(FlexValue::point(8));

    _layer->updateLayoutAttributesForLayer(_layer).setPadding(YGEdgeAll, FlexValue::point(8));

    auto size =
        _layer->sizeThatFits(Size::make(std::numeric_limits<Scalar>::max(), std::numeric_limits<Scalar>::max()));

    ASSERT_EQ(Size::make(32, 24), size);
}

class TestLayerWithSize : public Layer {
public:
    explicit TestLayerWithSize(const Ref<Resources>& resources) : Layer(resources) {}

    Size sizeThatFits(Size maxSize) override {
        return _intrinsicContentSize;
    }

    void setIntrinsicContentSize(Size value) {
        _intrinsicContentSize = value;
    }

private:
    Size _intrinsicContentSize;
};

TEST_F(FlexboxLayerTests, respectsSizeThatFitsOnChildLayer) {
    auto child = makeLayer<TestLayerWithSize>(_resources);

    _layer->addChild(child);

    _layer->updateLayoutAttributes().setFlexDirection(YGFlexDirectionRow);
    ;

    child->setIntrinsicContentSize(Size::make(4, 16));

    _layer->updateLayoutAttributesForLayer(_layer).setPadding(YGEdgeAll, FlexValue::point(8));

    auto size =
        _layer->sizeThatFits(Size::make(std::numeric_limits<Scalar>::max(), std::numeric_limits<Scalar>::max()));

    ASSERT_EQ(Size::make(20, 32), size);
}

TEST_F(FlexboxLayerTests, respectsSetNeedsLayout) {
    auto child = makeLayer<TestLayerWithSize>(_resources);

    _layer->addChild(child);

    _layer->updateLayoutAttributes().setFlexDirection(YGFlexDirectionRow);
    ;

    child->setIntrinsicContentSize(Size::make(4, 16));

    _layer->updateLayoutAttributesForLayer(_layer).setPadding(YGEdgeAll, FlexValue::point(8));

    auto size =
        _layer->sizeThatFits(Size::make(std::numeric_limits<Scalar>::max(), std::numeric_limits<Scalar>::max()));

    ASSERT_EQ(Size::make(20, 32), size);

    child->setIntrinsicContentSize(Size::make(6, 20));

    size = _layer->sizeThatFits(Size::make(std::numeric_limits<Scalar>::max(), std::numeric_limits<Scalar>::max()));

    // Shouldn't change until setNeedsLayout() is called
    ASSERT_EQ(Size::make(20, 32), size);

    child->setNeedsLayout();

    size = _layer->sizeThatFits(Size::make(std::numeric_limits<Scalar>::max(), std::numeric_limits<Scalar>::max()));

    ASSERT_EQ(Size::make(22, 36), size);
}

} // namespace snap::drawing
