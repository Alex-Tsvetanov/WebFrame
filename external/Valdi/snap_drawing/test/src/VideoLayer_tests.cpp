#include <gtest/gtest.h>

#include "TestBitmap.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/BitmapGraphicsContext.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"
#include "snap_drawing/cpp/Layers/VideoLayer.hpp"
#include "snap_drawing/cpp/Utils/ImageQueue.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

using namespace Valdi;

namespace snap::drawing {

class VideoLayerTests : public ::testing::Test {
protected:
    void SetUp() override {
        auto resources = makeShared<Resources>(nullptr, 1.0f, ConsoleLogger::getLogger());

        _layerRoot = makeLayer<LayerRoot>(resources);
        _videoLayer = makeLayer<VideoLayer>(resources);
        _layerRoot->setContentLayer(_videoLayer, ContentLayerSizingModeMatchSize);
        _layerRoot->setSize(Size(1.0f, 1.0f), 1.0f);
    }

    void TearDown() override {
        _videoLayer = nullptr;
        _layerRoot = nullptr;
    }

    Color updateAndDraw() {
        _layerRoot->processFrame(TimePoint::fromSeconds(_currentTime));

        BitmapGraphicsContext context;

        auto bitmap = makeShared<TestBitmap>(1, 1);
        auto surface = context.createBitmapSurface(bitmap);
        _layerRoot->drawInCanvas(surface->prepareCanvas().value());

        _currentTime += 1.0;

        return bitmap->getPixel(0, 0);
    }

    void enqueueColorToQueue(const Ref<ImageQueue>& queue, Color color) const {
        auto coloredBitmap = makeShared<TestBitmap>(1, 1);
        coloredBitmap->setPixel(0, 0, color);

        queue->enqueue(Image::makeFromBitmap(coloredBitmap, false).value());
    }

    Ref<LayerRoot> _layerRoot;
    Ref<VideoLayer> _videoLayer;
    TimeInterval _currentTime = 0.0;
};

TEST_F(VideoLayerTests, displaysNothingWhenQueueIsEmpty) {
    ASSERT_TRUE(_layerRoot->needsProcessFrame());
    auto color = updateAndDraw();
    ASSERT_FALSE(_layerRoot->needsProcessFrame());
    ASSERT_EQ(Color::transparent(), color);
}

TEST_F(VideoLayerTests, alwaysNeedsProcessFrameWhenQueueIsSet) {
    auto queue = makeShared<ImageQueue>(1);
    _videoLayer->setImageQueue(queue);

    ASSERT_TRUE(_layerRoot->needsProcessFrame());
    auto color = updateAndDraw();
    ASSERT_TRUE(_layerRoot->needsProcessFrame());
    ASSERT_EQ(Color::transparent(), color);

    color = updateAndDraw();
    ASSERT_TRUE(_layerRoot->needsProcessFrame());
    ASSERT_EQ(Color::transparent(), color);
}

TEST_F(VideoLayerTests, canDrawFromImageQueue) {
    auto queue = makeShared<ImageQueue>(1);
    _videoLayer->setImageQueue(queue);

    auto color = updateAndDraw();
    ASSERT_EQ(Color::transparent(), color);

    ASSERT_EQ(static_cast<size_t>(0), queue->getQueueSize());

    enqueueColorToQueue(queue, Color::red());

    ASSERT_EQ(static_cast<size_t>(1), queue->getQueueSize());

    color = updateAndDraw();
    ASSERT_EQ(Color::red(), color);

    ASSERT_EQ(static_cast<size_t>(0), queue->getQueueSize());

    ASSERT_TRUE(_layerRoot->needsProcessFrame());

    color = updateAndDraw();
    // Should remain the same
    ASSERT_EQ(Color::red(), color);
}

TEST_F(VideoLayerTests, dequeuesFromImageQueueRepeatedly) {
    auto queue = makeShared<ImageQueue>(3);
    _videoLayer->setImageQueue(queue);

    enqueueColorToQueue(queue, Color::red());
    enqueueColorToQueue(queue, Color::green());
    enqueueColorToQueue(queue, Color::blue());

    ASSERT_EQ(static_cast<size_t>(3), queue->getQueueSize());

    auto color = updateAndDraw();
    ASSERT_EQ(Color::red(), color);
    ASSERT_EQ(static_cast<size_t>(2), queue->getQueueSize());

    color = updateAndDraw();
    ASSERT_EQ(Color::green(), color);
    ASSERT_EQ(static_cast<size_t>(1), queue->getQueueSize());

    color = updateAndDraw();
    ASSERT_EQ(Color::blue(), color);
    ASSERT_EQ(static_cast<size_t>(0), queue->getQueueSize());

    color = updateAndDraw();
    ASSERT_EQ(Color::blue(), color);
}

TEST_F(VideoLayerTests, stopsProcessingWhenQueueIsRemoved) {
    auto queue = makeShared<ImageQueue>(1);
    _videoLayer->setImageQueue(queue);

    ASSERT_TRUE(_layerRoot->needsProcessFrame());
    updateAndDraw();

    ASSERT_TRUE(_layerRoot->needsProcessFrame());
    updateAndDraw();

    _videoLayer->setImageQueue(nullptr);
    updateAndDraw();

    ASSERT_FALSE(_layerRoot->needsProcessFrame());

    _videoLayer->setImageQueue(queue);
    ASSERT_TRUE(_layerRoot->needsProcessFrame());
}

} // namespace snap::drawing
