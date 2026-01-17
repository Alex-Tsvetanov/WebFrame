#include "DisplayListBuilder.hpp"
#include "snap_drawing/cpp/Drawing/Raster/RasterDamageResolver.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace snap::drawing {

class RasterDamageResolverTests : public ::testing::Test {
protected:
    DisplayListBuilder _builder = DisplayListBuilder(100, 100);
    RasterDamageResolver _damageResolver;

    void SetUp() override {
        _builder = DisplayListBuilder(100, 100);
        _damageResolver = RasterDamageResolver();
    }

    std::vector<Rect> resolveDamage() {
        _damageResolver.beginUpdates(100, 100);
        _damageResolver.addDamageFromDisplayListUpdates(*_builder.displayList);
        return _damageResolver.endUpdates();
    }
};

TEST_F(RasterDamageResolverTests, returnsFullRectOnInInitialDraw) {
    _builder.context(Vector(0, 0), 1.0, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
    });
    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(1), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(0, 0, 100, 100), damageRects[0]);
}

TEST_F(RasterDamageResolverTests, returnsPartialDamageRect) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, true, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();
    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(1), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(50, 50, 10, 10), damageRects[0]);
}

TEST_F(RasterDamageResolverTests, returnsMultipleDamageRects) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, true, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, true, [&]() { _builder.rectangle(Size(15, 15), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();
    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(2), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(50, 50, 10, 10), damageRects[0]);
    ASSERT_EQ(Rect::makeXYWH(20, 20, 15, 15), damageRects[1]);
}

TEST_F(RasterDamageResolverTests, mergesMultipleDamageRectsWhenPossible) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, true, [&]() { _builder.rectangle(Size(20, 20), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, true, [&]() { _builder.rectangle(Size(40, 40), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();
    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(1), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(20, 20, 50, 50), damageRects[0]);
}

TEST_F(RasterDamageResolverTests, returnsEmptyDamageRectsWhenNoDamage) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, false, [&]() { _builder.rectangle(Size(50, 50), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();
    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(0), damageRects.size());
}

TEST_F(RasterDamageResolverTests, returnsDamageOnInsertedLayer) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, false, [&]() { _builder.rectangle(Size(50, 50), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();

    _builder = DisplayListBuilder(100, 100);
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, false, [&]() { _builder.rectangle(Size(50, 50), 1.0); });
        _builder.context(Vector(10, 10), 1.0, 4, true, [&]() { _builder.rectangle(Size(15, 15), 1.0); });
    });

    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(1), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(10, 10, 15, 15), damageRects[0]);
}

TEST_F(RasterDamageResolverTests, returnsDamageOnRemovedLayer) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, false, [&]() { _builder.rectangle(Size(50, 50), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();

    _builder = DisplayListBuilder(100, 100);
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
    });

    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(1), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(20, 20, 50, 50), damageRects[0]);
}

TEST_F(RasterDamageResolverTests, returnsDamageOnMovedLayer) {
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(50, 50), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, false, [&]() { _builder.rectangle(Size(50, 50), 1.0); });
    });

    // First pass to populate the previous layer contents
    resolveDamage();

    _builder = DisplayListBuilder(100, 100);
    _builder.context(Vector(0, 0), 1.0, 1, false, [&]() {
        _builder.rectangle(Size(100, 100), 1.0);
        _builder.context(Vector(10, 10), 1.0, 2, false, [&]() { _builder.rectangle(Size(10, 10), 1.0); });
        _builder.context(Vector(20, 20), 1.0, 3, false, [&]() { _builder.rectangle(Size(50, 50), 1.0); });
    });

    auto damageRects = resolveDamage();

    ASSERT_EQ(static_cast<size_t>(2), damageRects.size());
    ASSERT_EQ(Rect::makeXYWH(50, 50, 10, 10), damageRects[0]);
    ASSERT_EQ(Rect::makeXYWH(10, 10, 10, 10), damageRects[1]);
}

} // namespace snap::drawing