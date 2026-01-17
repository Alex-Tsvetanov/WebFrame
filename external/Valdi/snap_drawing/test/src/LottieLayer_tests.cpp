#include <gtest/gtest.h>

#include "TestBitmap.hpp"
#include "snap_drawing/cpp/Layers/AnimatedImageLayer.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"
#include "snap_drawing/cpp/Text/FontManager.hpp"
#include "snap_drawing/cpp/Utils/AnimatedImage.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

#include "TestDataUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

constexpr int kExpectedAnimationDurationMs = 3903;

class TestLayerRootListener : public LayerRootListener {
public:
    bool didDraw = false;
    void onNeedsProcessFrame(LayerRoot& /*root*/) override {}

    void onDidDraw(LayerRoot& root,
                   const Ref<DisplayList>& displayList,
                   const CompositorPlaneList* planeList) override {
        didDraw = true;
    }
};

class AnimatedImageLayerTests : public ::testing::Test {
protected:
    void SetUp() override {
        auto fontManager = makeShared<FontManager>(ConsoleLogger::getLogger());
        auto resources = makeShared<Resources>(fontManager, 1.0f, ConsoleLogger::getLogger());

        _layerRoot = makeLayer<LayerRoot>(resources);
        _animatedImageLayer = makeLayer<AnimatedImageLayer>(resources);
        _layerRoot->setContentLayer(_animatedImageLayer, ContentLayerSizingModeMatchSize);
        _layerRoot->setSize(Size(1.0f, 1.0f), 1.0f);
        _layerRoot->setListener(&_layerRootListener);

        auto testData = getTestData("lottie_loading.json");
        SC_ASSERT(testData.success(), testData.description());

        auto animatedImage = AnimatedImage::make(fontManager, testData.value().data(), testData.value().size());
        SC_ASSERT(animatedImage.success(), animatedImage.description());
        _animatedImage = animatedImage.moveValue();

        ASSERT_EQ(kExpectedAnimationDurationMs, static_cast<int>(_animatedImage->getDuration().milliseconds()));
    }

    void TearDown() override {
        _animatedImageLayer = nullptr;
        _layerRoot = nullptr;
        _animatedImage = nullptr;
    }

    bool update(double delta) {
        _layerRootListener.didDraw = false;
        _layerRoot->processFrame(TimePoint::fromSeconds(_currentTime));

        _currentTime += delta;

        return _layerRootListener.didDraw;
    }

    Ref<LayerRoot> _layerRoot;
    Ref<AnimatedImageLayer> _animatedImageLayer;
    Ref<AnimatedImage> _animatedImage;
    TimeInterval _currentTime = 0.0;
    TestLayerRootListener _layerRootListener;
};

TEST_F(AnimatedImageLayerTests, setsAnimationWhenSceneIsSetWithAdvanceRate) {
    update(0.5);

    ASSERT_FALSE(_animatedImageLayer->needsProcessAnimations());

    _animatedImageLayer->setImage(_animatedImage);

    ASSERT_EQ(static_cast<size_t>(0), _animatedImageLayer->getAnimationKeys().size());
    ASSERT_FALSE(_animatedImageLayer->needsProcessAnimations());

    _animatedImageLayer->setAdvanceRate(1.0);

    ASSERT_EQ(static_cast<size_t>(1), _animatedImageLayer->getAnimationKeys().size());
    ASSERT_TRUE(_animatedImageLayer->needsProcessAnimations());

    ASSERT_TRUE(update(0.5));

    ASSERT_EQ(static_cast<size_t>(1), _animatedImageLayer->getAnimationKeys().size());
    ASSERT_TRUE(_animatedImageLayer->needsProcessAnimations());

    _animatedImageLayer->setImage(nullptr);

    ASSERT_EQ(static_cast<size_t>(0), _animatedImageLayer->getAnimationKeys().size());
    ASSERT_TRUE(_animatedImageLayer->needsProcessAnimations());

    ASSERT_TRUE(update(0.5));

    ASSERT_EQ(static_cast<size_t>(0), _animatedImageLayer->getAnimationKeys().size());
    ASSERT_FALSE(_animatedImageLayer->needsProcessAnimations());

    _animatedImageLayer->setImage(_animatedImage);

    ASSERT_EQ(static_cast<size_t>(1), _animatedImageLayer->getAnimationKeys().size());
    ASSERT_TRUE(_animatedImageLayer->needsProcessAnimations());
}

TEST_F(AnimatedImageLayerTests, canSeek) {
    _animatedImageLayer->setImage(_animatedImage);

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    _animatedImageLayer->setCurrentTime(Duration::fromMilliseconds(250));
    // Should only be impacted on draw
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(250, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    _animatedImageLayer->setCurrentTime(Duration::fromMilliseconds(750));
    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(750, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    _animatedImageLayer->setCurrentTime(Duration::fromMilliseconds(100));
    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(100, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

TEST_F(AnimatedImageLayerTests, canMoveForward) {
    _animatedImageLayer->setImage(_animatedImage);
    _animatedImageLayer->setAdvanceRate(1.0);

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.25));
    ASSERT_EQ(500, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(750, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    _animatedImageLayer->setAdvanceRate(3.0);

    ASSERT_TRUE(update(0.1));
    ASSERT_EQ(2250, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(1.0));
    ASSERT_EQ(2550, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

TEST_F(AnimatedImageLayerTests, canMoveBackward) {
    _animatedImageLayer->setImage(_animatedImage);
    _animatedImageLayer->setAdvanceRate(-1.0);
    _animatedImageLayer->setCurrentTime(Duration::fromMilliseconds(3500));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(3500, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.25));
    ASSERT_EQ(3000, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(2750, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    _animatedImageLayer->setAdvanceRate(-3.0);

    ASSERT_TRUE(update(0.1));
    ASSERT_EQ(1250, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

TEST_F(AnimatedImageLayerTests, staysAtLastFrameWhenReachingEndForward) {
    _animatedImageLayer->setImage(_animatedImage);
    _animatedImageLayer->setAdvanceRate(1.0);

    ASSERT_TRUE(update(3.0));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(2.0));
    ASSERT_EQ(3000, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(3903, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_FALSE(update(0.5));
    ASSERT_EQ(3903, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_FALSE(update(0.5));
    ASSERT_EQ(3903, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

TEST_F(AnimatedImageLayerTests, staysAtFirstFrameWhenReachingEndForward) {
    _animatedImageLayer->setImage(_animatedImage);
    _animatedImageLayer->setAdvanceRate(-1.0);
    _animatedImageLayer->setCurrentTime(Duration::fromMilliseconds(3500));

    ASSERT_TRUE(update(3.0));
    ASSERT_EQ(3500, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(2.0));
    ASSERT_EQ(500, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_FALSE(update(0.5));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_FALSE(update(0.5));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

TEST_F(AnimatedImageLayerTests, canLoopForward) {
    _animatedImageLayer->setImage(_animatedImage);
    _animatedImageLayer->setAdvanceRate(1.0);
    _animatedImageLayer->setShouldLoop(true);

    ASSERT_TRUE(update(2.0));
    ASSERT_EQ(0, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(1.0));
    ASSERT_EQ(2000, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(2));
    ASSERT_EQ(3000, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.1));
    ASSERT_EQ(1096, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

TEST_F(AnimatedImageLayerTests, canLoopBackward) {
    _animatedImageLayer->setImage(_animatedImage);
    _animatedImageLayer->setAdvanceRate(-1.0);
    _animatedImageLayer->setShouldLoop(true);
    _animatedImageLayer->setCurrentTime(Duration::fromMilliseconds(1000));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(1000, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.75));
    ASSERT_EQ(500, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));

    ASSERT_TRUE(update(0.5));
    ASSERT_EQ(3653, static_cast<int>(_animatedImage->getCurrentTime().milliseconds()));
}

} // namespace snap::drawing
