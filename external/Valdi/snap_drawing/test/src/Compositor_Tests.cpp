#include <gtest/gtest.h>

#include "DisplayListBuilder.hpp"
#include "snap_drawing/cpp/Drawing/Composition/Compositor.hpp"
#include "snap_drawing/cpp/Drawing/Composition/CompositorPlaneList.hpp"
#include "snap_drawing/cpp/Drawing/DisplayList/DisplayList.hpp"
#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "src/base/SkFloatBits.h"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"

using namespace Valdi;

namespace snap::drawing {

static CompositorPlaneList performComposition(DisplayListBuilder& builder,
                                              Ref<DisplayList>* outputDisplayList = nullptr) {
    CompositorPlaneList planeList;
    Compositor compositor(ConsoleLogger::getLogger());
    auto displayList = compositor.performComposition(*builder.displayList, planeList);
    if (outputDisplayList != nullptr) {
        *outputDisplayList = displayList;
    }
    return planeList;
}

TEST(Compositor, doesNothingWhenNoExternalSurfacesArePresent) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(50, 50), 1.0);

        builder.context(Vector(25, 25), 1.0, [&]() { builder.rectangle(Size(10, 10), 1.0); });

        builder.context(Vector(35, 35), 1.0, [&]() { builder.rectangle(Size(50, 50), 1.0); });

        builder.rectangle(Size(25, 25), 1.0);
    });

    Ref<DisplayList> outputDisplayList;
    auto planeList = performComposition(builder, &outputDisplayList);

    ASSERT_EQ(static_cast<size_t>(1), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_TRUE(planeList.getPlaneAtIndex(0).getExternalSurface() == nullptr);

    ASSERT_EQ(builder.displayList, outputDisplayList);
}

TEST(Compositor, emitsDedicatedLayerWhenAnExternalSurfaceIsPresent) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(50, 50), 1.0);

        builder.context(Vector(25, 25), 1.0, [&]() { builder.rectangle(Size(10, 10), 1.0); });

        builder.context(Vector(35, 35), 1.0, [&]() { builder.rectangle(Size(50, 50), 1.0); });

        builder.rectangle(Size(25, 25), 1.0);

        builder.externalSurface(Size(50, 50), 1.0);
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, resolvesDisplayStateOfExternalSurface) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(Vector(10, 20), 0.75, [&]() {
            builder.context(Vector(0, 0), 1.0, [&]() {
                builder.context(Vector(30, 40), 0.5, [&]() { builder.externalSurface(Size(50, 50), 0.25); });
            });
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(1), planeList.getPlanesCount());

    const auto& plane = planeList.getPlaneAtIndex(0);
    ASSERT_EQ(CompositorPlaneTypeExternal, plane.getType());
    ASSERT_TRUE(plane.getExternalSurface() != nullptr);

    Matrix expectedTransform;

    ASSERT_EQ(Rect::makeXYWH(40, 60, 50, 50), plane.getExternalSurfacePresenterState()->frame);
    ASSERT_EQ(expectedTransform, plane.getExternalSurfacePresenterState()->transform);
    ASSERT_EQ(0.09375, plane.getExternalSurfacePresenterState()->opacity);
}

TEST(Compositor, resolvesDisplayStateOfExternalSurfaceWithComplexTransforms) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        Matrix outerMatrix;
        outerMatrix.setTranslateX(10);
        outerMatrix.setTranslateY(20);
        outerMatrix.setScaleX(2.0);
        outerMatrix.setScaleY(4.0);

        builder.context(outerMatrix, 1.0, [&]() {
            builder.context(Vector(0, 0), 1.0, [&]() {
                Matrix innerMatrix;
                innerMatrix.postRotateDegrees(90.0, 25.0, 25.0);
                builder.context(innerMatrix, 1.0, [&]() { builder.externalSurface(Size(50, 50), 1.0); });
            });
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(1), planeList.getPlanesCount());

    const auto& plane = planeList.getPlaneAtIndex(0);
    ASSERT_EQ(CompositorPlaneTypeExternal, plane.getType());
    ASSERT_TRUE(plane.getExternalSurface() != nullptr);

    Matrix expectedTransform;
    expectedTransform.setScaleX(0);
    expectedTransform.setScaleY(0);
    expectedTransform.setTranslateX(110);
    expectedTransform.setTranslateY(20);
    expectedTransform.setSkewX(-2);
    expectedTransform.setSkewY(4);

    ASSERT_EQ(Rect::makeXYWH(0, 0, 50, 50), plane.getExternalSurfacePresenterState()->frame);
    ASSERT_EQ(
        Rect::makeXYWH(10, 20, 100, 200),
        plane.getExternalSurfacePresenterState()->transform.mapRect(plane.getExternalSurfacePresenterState()->frame));
    ASSERT_EQ(expectedTransform, plane.getExternalSurfacePresenterState()->transform);
}

TEST(Compositor, resolvesSimpleClipOfExternalSurface) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.clip(Size(100, 100));
        builder.context(Vector(10, 20), 0.75, [&]() {
            builder.context(Vector(0, 0), 1.0, [&]() {
                builder.context(Vector(30, 40), 0.5, [&]() {
                    builder.clip(Size(20, 13));
                    builder.externalSurface(Size(50, 50), 0.25);
                });
            });
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(1), planeList.getPlanesCount());

    const auto& plane = planeList.getPlaneAtIndex(0);
    ASSERT_EQ(CompositorPlaneTypeExternal, plane.getType());
    ASSERT_TRUE(plane.getExternalSurface() != nullptr);

    Path expectedClip;
    expectedClip.addRect(Rect::makeXYWH(40, 60, 20, 13), true);

    ASSERT_EQ(expectedClip, plane.getExternalSurfacePresenterState()->clipPath);
}

TEST(Compositor, resolvesComplexClipOfExternalSurface) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.clip(BorderRadius::makeCircle(), Size(100, 100));
        builder.context(Vector(50, 50), 1.0, [&]() {
            builder.clip(Size(50, 25));
            builder.externalSurface(Size(50, 50), 0.25);
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(1), planeList.getPlanesCount());

    const auto& plane = planeList.getPlaneAtIndex(0);
    ASSERT_EQ(CompositorPlaneTypeExternal, plane.getType());
    ASSERT_TRUE(plane.getExternalSurface() != nullptr);

    Path expectedClip;
    expectedClip.moveTo(SkBits2Float(0x42ba9a40), 75); // 93.3013f, 75
    expectedClip.lineTo(50, 75);
    expectedClip.lineTo(50, 50);
    expectedClip.lineTo(100, 50);
    expectedClip.conicTo(100,
                         SkBits2Float(0x427d9700),
                         SkBits2Float(0x42ba9a40),
                         75,
                         SkBits2Float(0x3f7746ea)); // 100, 63.3975f, 93.3013f, 75, 0.965926f
    expectedClip.close();

    ASSERT_EQ(expectedClip, plane.getExternalSurfacePresenterState()->clipPath);
}

TEST(Compositor, usesOneLayerPerExternalSurface) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.externalSurface(Size(10, 10), 1.0);

        builder.context(Vector(25, 25), 1.0, [&]() { builder.externalSurface(Size(20, 20), 1.0); });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_TRUE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, supportsExternalSurfaceOnTopOfRegularLayer) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(25, 25), 1.0);
        builder.context(Vector(100, 100), 1.0, [&]() { builder.rectangle(Size(1, 1), 1.0); });

        builder.externalSurface(Size(10, 10), 1.0);
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_FALSE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, supportsRegularLayerOnTopOfExternalSurface) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.externalSurface(Size(10, 10), 1.0);

        builder.rectangle(Size(25, 25), 1.0);
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(1).getType());
    ASSERT_TRUE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_FALSE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, supportsExternalSurfaceInBetweenRegularLayers) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(25, 25), 1.0);

        builder.context(Vector(0.0, 0.0), 1.0, [&]() { builder.externalSurface(Size(10, 10), 1.0); });

        builder.rectangle(Size(15, 15), 1.0);
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(3), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(2).getType());
    ASSERT_FALSE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
    ASSERT_FALSE(planeList.getPlaneAtIndex(2).getExternalSurface() != nullptr);
}

TEST(Compositor, avoidsCreatingRegularLayerIfItemFitsInALowerRegularLayer) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(25, 25), 1.0);

        builder.context(Vector(15, 15), 1.0, [&]() { builder.externalSurface(Size(10, 10), 1.0); });

        builder.rectangle(Size(15, 15), 1.0);
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_FALSE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, takesInAccountClippingWhenCalculatingRegularLayerFit) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(25, 25), 1.0);

        builder.context(Vector(15, 15), 1.0, [&]() { builder.externalSurface(Size(10, 10), 1.0); });

        builder.context(Vector(0, 0), 1.0, [&]() {
            builder.clip(Size(15, 15));
            builder.rectangle(Size(100, 100), 1.0);
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_FALSE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, handlesClipThatDoesntImpactLayerFit) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(25, 25), 1.0);

        builder.context(Vector(15, 15), 1.0, [&]() { builder.externalSurface(Size(10, 10), 1.0); });

        builder.context(Vector(0, 0), 1.0, [&]() {
            builder.clip(Size(50, 16));
            builder.rectangle(Size(100, 100), 1.0);
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(3), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(2).getType());
}

TEST(Compositor, mergesClips) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.rectangle(Size(25, 25), 1.0);

        builder.context(Vector(15, 15), 1.0, [&]() { builder.externalSurface(Size(10, 10), 1.0); });

        builder.context(Vector(0, 0), 1.0, [&]() {
            builder.clip(Size(15, 100));

            builder.context(Vector(0, 0), 1.0, [&]() {
                builder.clip(Size(50, 50));

                builder.rectangle(Size(100, 100), 1.0);
            });
        });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_FALSE(planeList.getPlaneAtIndex(0).getExternalSurface() != nullptr);
    ASSERT_TRUE(planeList.getPlaneAtIndex(1).getExternalSurface() != nullptr);
}

TEST(Compositor, handlesComplexCompositions) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(Vector(0.0, 0.0), 1.0, [&]() { builder.rectangle(Size(25, 25), 1.0); });

        builder.context(Vector(10.0, 10.0), 1.0, [&]() { builder.externalSurface(Size(25, 25), 1.0); });

        builder.context(Vector(20.0, 20.0), 1.0, [&]() { builder.rectangle(Size(25, 25), 1.0); });

        builder.context(Vector(30.0, 30.0), 1.0, [&]() { builder.externalSurface(Size(25, 25), 1.0); });

        builder.context(Vector(40.0, 40.0), 1.0, [&]() { builder.rectangle(Size(25, 25), 1.0); });

        builder.context(Vector(50.0, 50.0), 1.0, [&]() { builder.externalSurface(Size(25, 25), 1.0); });

        // This one specifically does not overlap with all external planes
        // and should be thus added to an existing layer instead of creating one
        builder.context(Vector(0.0, 0.0), 1.0, [&]() { builder.rectangle(Size(25, 25), 1.0); });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(6), planeList.getPlanesCount());

    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(2).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(3).getType());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(4).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(5).getType());
}

TEST(Compositor, keepsExternalSurfaceBelowOverlayWhenPossible) {
    DisplayListBuilder builder(100, 100);

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(Vector(0.0, 0.0), 1.0, [&]() { builder.rectangle(Size(25, 25), 1.0); });

        builder.context(Vector(10.0, 10.0), 1.0, [&]() { builder.externalSurface(Size(25, 25), 1.0); });

        builder.context(Vector(10.0, 25.0), 1.0, [&]() { builder.rectangle(Size(25, 2), 1.0); });

        builder.context(Vector(5.0, 50.0), 1.0, [&]() { builder.externalSurface(Size(25, 25), 1.0); });

        builder.context(Vector(0.0, 65.0), 1.0, [&]() { builder.rectangle(Size(25, 2), 1.0); });
    });

    auto planeList = performComposition(builder);

    ASSERT_EQ(static_cast<size_t>(4), planeList.getPlanesCount());

    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(0).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(1).getType());
    ASSERT_EQ(CompositorPlaneTypeExternal, planeList.getPlaneAtIndex(2).getType());
    ASSERT_EQ(CompositorPlaneTypeDrawable, planeList.getPlaneAtIndex(3).getType());
}

TEST(Compositor, buildsOutputDisplayList) {
    DisplayListBuilder builder(100, 100);

    LayerContent firstRectangle;
    LayerContent secondRectangle;

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(Vector(1, 1), 1.0, [&]() {
            builder.clip(Size(100, 100));

            builder.context(Vector(-1, -1), 1.0, [&]() { firstRectangle = builder.rectangle(Size(25, 25), 1.0); });

            builder.context(Vector(0.0, 0.0), 1.0, [&]() {
                builder.clip(Size(10, 10));
                builder.externalSurface(Size(10, 10), 1.0);
            });

            builder.context(Vector(0, 0), 1.0, [&]() { secondRectangle = builder.rectangle(Size(15, 15), 1.0); });
        });
    });

    Ref<DisplayList> outputDisplayList;
    auto planeList = performComposition(builder, &outputDisplayList);
    ASSERT_EQ(static_cast<size_t>(3), planeList.getPlanesCount());

    DisplayListBuilder expectedBuilder(100, 100);
    expectedBuilder.displayList->removePlane(0);

    expectedBuilder.plane([&]() {
        expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
            expectedBuilder.context(Vector(1, 1), 1.0, [&]() {
                expectedBuilder.clip(Size(100, 100));
                expectedBuilder.context(
                    Vector(-1, -1), 1.0, [&]() { expectedBuilder.layerContent(firstRectangle, 1.0); });
            });
        });
    });

    expectedBuilder.plane([&]() {
        expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
            expectedBuilder.context(Vector(1, 1), 1.0, [&]() {
                expectedBuilder.clip(Size(100, 100));
                expectedBuilder.context(
                    Vector(0, 0), 1.0, [&]() { expectedBuilder.layerContent(secondRectangle, 1.0); });
            });
        });
    });

    ASSERT_EQ(*expectedBuilder.displayList, *outputDisplayList);
}

TEST(Compositor, canHandleMultipleClipsInContext) {
    DisplayListBuilder builder(100, 100);

    LayerContent firstRectangle;
    LayerContent secondRectangle;

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(Vector(1, 1), 1.0, [&]() {
            builder.clip(Size(100, 100));

            builder.context(Vector(-1, -1), 1.0, [&]() { firstRectangle = builder.rectangle(Size(25, 25), 1.0); });

            builder.clip(Size(50, 50));

            secondRectangle = builder.rectangle(Size(25, 25), 1.0);

            builder.context(Vector(0.0, 0.0), 1.0, [&]() {
                builder.clip(Size(10, 10));
                builder.externalSurface(Size(10, 10), 1.0);
            });
        });
    });

    Ref<DisplayList> outputDisplayList;
    auto planeList = performComposition(builder, &outputDisplayList);
    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());

    DisplayListBuilder expectedBuilder(100, 100);
    expectedBuilder.displayList->removePlane(0);

    expectedBuilder.plane([&]() {
        expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
            expectedBuilder.context(Vector(1, 1), 1.0, [&]() {
                expectedBuilder.clip(Size(100, 100));
                expectedBuilder.context(
                    Vector(-1, -1), 1.0, [&]() { expectedBuilder.layerContent(firstRectangle, 1.0); });
                expectedBuilder.clip(Size(50, 50));
                expectedBuilder.layerContent(secondRectangle, 1.0);
            });
        });
    });

    ASSERT_EQ(*expectedBuilder.displayList, *outputDisplayList);
}

TEST(Compositor, appliesMaskToActivePlane) {
    DisplayListBuilder builder(100, 100);

    LayerContent firstRectangle;
    LayerContent secondRectangle;

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(Vector(0, 0), 1.0, [&]() {
            builder.mask(Rect::makeXYWH(10, 10, 10, 10), [&]() {
                firstRectangle = builder.rectangle(Size(25, 25), 1.0);

                builder.context(Vector(0.0, 0.0), 1.0, [&]() {
                    builder.externalSurface(Size(30, 30), 1.0);

                    builder.mask(Rect::makeXYWH(15, 15, 30, 30),
                                 [&]() { secondRectangle = builder.rectangle(Size(25, 25), 1.0); });
                });
            });
        });
    });

    Ref<DisplayList> outputDisplayList;
    auto planeList = performComposition(builder, &outputDisplayList);
    ASSERT_EQ(static_cast<size_t>(3), planeList.getPlanesCount());

    DisplayListBuilder expectedBuilder(100, 100);
    expectedBuilder.displayList->removePlane(0);

    expectedBuilder.plane([&]() {
        expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
            expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
                expectedBuilder.mask(Rect::makeXYWH(10, 10, 10, 10),
                                     [&]() { expectedBuilder.layerContent(firstRectangle, 1.0); });
            });
        });
    });

    expectedBuilder.plane([&]() {
        expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
            expectedBuilder.context(Vector(0, 0), 1.0, [&]() {
                expectedBuilder.context(Vector(0.0, 0.0), 1.0, [&]() {
                    expectedBuilder.mask(Rect::makeXYWH(15, 15, 30, 30),
                                         [&]() { expectedBuilder.layerContent(secondRectangle, 1.0); });
                });
            });
        });
    });

    ASSERT_EQ(*expectedBuilder.displayList, *outputDisplayList);
}

TEST(Compositor, optimizesOutUnusedCommands) {
    DisplayListBuilder builder(100, 100);

    LayerContent rectangle;

    builder.context(Vector(0, 0), 1.0, [&]() {
        builder.context(
            Vector(5, 5), 1.0, [&]() { builder.context(Vector(0, 0), 0.5, [&]() { builder.clip(Size(100, 100)); }); });

        rectangle = builder.rectangle(Size(25, 25), 1.0);

        builder.context(Vector(5, 5), 1.0, [&]() { builder.clip(Size(100, 100)); });

        builder.externalSurface(Size(10, 10), 1.0);
    });

    Ref<DisplayList> outputDisplayList;
    auto planeList = performComposition(builder, &outputDisplayList);

    ASSERT_EQ(static_cast<size_t>(2), planeList.getPlanesCount());

    DisplayListBuilder expectedBuilder(100, 100);

    expectedBuilder.context(Vector(0, 0), 1.0, [&]() { expectedBuilder.layerContent(rectangle, 1.0); });

    ASSERT_EQ(*expectedBuilder.displayList, *outputDisplayList);
}

} // namespace snap::drawing
