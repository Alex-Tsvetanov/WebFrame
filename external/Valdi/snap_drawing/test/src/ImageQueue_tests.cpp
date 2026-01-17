#include <gtest/gtest.h>

#include "TestBitmap.hpp"

#include "snap_drawing/cpp/Utils/ImageQueue.hpp"

using namespace Valdi;

namespace snap::drawing {

class ImageQueueTests : public ::testing::Test {
protected:
    void SetUp() override {
        _imageQueue = makeShared<ImageQueue>(2);
    }

    Ref<IBitmap> allocateAndEnqueue(int width, int height, ColorType colorType) {
        auto result = _imageQueue->allocateBitmap(width, height, colorType);

        auto bitmap = result.moveValue();
        _imageQueue->enqueue(Image::makeFromBitmap(bitmap, false).value());

        return bitmap;
    }

    Ref<ImageQueue> _imageQueue;
};

TEST_F(ImageQueueTests, canEnqueueAndDequeue) {
    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getQueueSize());

    auto bitmap1 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(1), _imageQueue->getQueueSize());

    auto image = _imageQueue->dequeue();

    ASSERT_TRUE(image.has_value());
    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getQueueSize());

    auto image2 = _imageQueue->dequeue();
    ASSERT_FALSE(image2.has_value());
}

TEST_F(ImageQueueTests, clearOutQueueWhenReachingLimit) {
    auto bitmap1 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(1), _imageQueue->getQueueSize());
    ASSERT_EQ(static_cast<long>(2), bitmap1.use_count());

    auto bitmap2 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(2), _imageQueue->getQueueSize());
    ASSERT_EQ(static_cast<long>(2), bitmap1.use_count());

    auto bitmap3 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(2), _imageQueue->getQueueSize());
    ASSERT_EQ(static_cast<long>(1), bitmap1.use_count());
}

TEST_F(ImageQueueTests, canClear) {
    allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    ASSERT_EQ(static_cast<size_t>(2), _imageQueue->getQueueSize());

    _imageQueue->clear();
    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getQueueSize());

    auto image = _imageQueue->dequeue();
    ASSERT_FALSE(image.has_value());
}

TEST_F(ImageQueueTests, canReuseBitmaps) {
    auto bitmap = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    void* bytes = bitmap->lockBytes();

    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());
    _imageQueue->dequeue();

    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());
    bitmap = nullptr;

    ASSERT_EQ(static_cast<size_t>(1), _imageQueue->getPoolSize());

    bitmap = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());

    ASSERT_EQ(bytes, bitmap->lockBytes());
}

TEST_F(ImageQueueTests, maintainsPoolSizeBelowThreshold) {
    auto bitmap = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    auto bitmap2 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    auto bitmap3 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    auto bitmap4 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());

    _imageQueue->clear();

    bitmap = nullptr;
    ASSERT_EQ(static_cast<size_t>(1), _imageQueue->getPoolSize());

    bitmap2 = nullptr;
    ASSERT_EQ(static_cast<size_t>(2), _imageQueue->getPoolSize());

    bitmap3 = nullptr;
    ASSERT_EQ(static_cast<size_t>(3), _imageQueue->getPoolSize());

    bitmap4 = nullptr;
    ASSERT_EQ(static_cast<size_t>(3), _imageQueue->getPoolSize());
}

TEST_F(ImageQueueTests, clearsPoolWhenSwitchingSizeOrColorType) {
    auto bitmap = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    auto bitmap2 = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);

    _imageQueue->clear();

    bitmap = nullptr;
    bitmap2 = nullptr;

    ASSERT_EQ(static_cast<size_t>(2), _imageQueue->getPoolSize());

    auto bitmap3 = allocateAndEnqueue(2, 2, ColorType::ColorTypeRGBA8888);

    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());

    auto bitmap4 = allocateAndEnqueue(2, 2, ColorType::ColorTypeRGBA8888);

    _imageQueue->clear();
    bitmap3 = nullptr;
    bitmap4 = nullptr;

    ASSERT_EQ(static_cast<size_t>(2), _imageQueue->getPoolSize());

    auto bitmap5 = allocateAndEnqueue(2, 2, ColorType::ColorTypeAlpha8);

    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());
}

TEST_F(ImageQueueTests, doesntEnqueueToPoolWhenSwitchingSizeOrColorType) {
    auto bitmap = allocateAndEnqueue(1, 1, ColorType::ColorTypeRGBA8888);
    auto bitmap2 = allocateAndEnqueue(2, 2, ColorType::ColorTypeRGBA8888);

    _imageQueue->clear();

    bitmap = nullptr;
    ASSERT_EQ(static_cast<size_t>(0), _imageQueue->getPoolSize());

    bitmap2 = nullptr;
    ASSERT_EQ(static_cast<size_t>(1), _imageQueue->getPoolSize());
}

} // namespace snap::drawing
