#include <gtest/gtest.h>

#include "snap_drawing/cpp/Animations/ValueInterpolators.hpp"
#include "snap_drawing/cpp/Utils/BorderRadius.hpp"

namespace snap::drawing {

TEST(InterpolatorTest, canInterpolatePercentBorderRadius) {
    auto fromBorderRadius = BorderRadius(10, 20, 30, 40, true, true, true, true);

    auto toBorderRadius = BorderRadius(14, 24, 34, 44, true, true, true, true);

    auto bounds = Rect::makeXYWH(0.f, 0.f, 100.f, 100.f);
    auto halfProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.5f);

    ASSERT_EQ("[12.0%, 22.0%, 32.0%, 42.0%]", halfProgressInterpolated.toString());

    auto threeQuartersProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.75f);

    ASSERT_EQ("[13.0%, 23.0%, 33.0%, 43.0%]", threeQuartersProgressInterpolated.toString());
}

TEST(InterpolatorTest, canInterpolatePointBorderRadius) {
    auto fromBorderRadius = BorderRadius(10, 20, 30, 40, false, false, false, false);

    auto toBorderRadius = BorderRadius(14, 24, 34, 44, false, false, false, false);

    auto bounds = Rect::makeXYWH(0.f, 0.f, 100.f, 100.f);
    auto halfProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.5f);

    ASSERT_EQ("[12.0, 22.0, 32.0, 42.0]", halfProgressInterpolated.toString());

    auto threeQuartersProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.75f);

    ASSERT_EQ("[13.0, 23.0, 33.0, 43.0]", threeQuartersProgressInterpolated.toString());
}

TEST(InterpolatorTest, canInterpolatePercentToPointBorderRadius) {
    auto fromBorderRadius = BorderRadius(20, 30, 40, 50, true, true, true, true);

    auto toBorderRadius = BorderRadius(20, 30, 40, 50, false, false, false, false);

    auto bounds = Rect::makeXYWH(0.f, 0.f, 200.f, 200.f);
    auto halfProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.5f);

    ASSERT_EQ("[30.0, 45.0, 60.0, 75.0]", halfProgressInterpolated.toString());

    auto threeQuartersProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.75f);

    ASSERT_EQ("[25.0, 37.5, 50.0, 62.5]", threeQuartersProgressInterpolated.toString());
}

TEST(InterpolatorTest, canInterpolatePointToPercentBorderRadius) {
    auto fromBorderRadius = BorderRadius(20, 30, 40, 50, false, false, false, false);

    auto toBorderRadius = BorderRadius(20, 30, 40, 50, true, true, true, true);

    auto bounds = Rect::makeXYWH(0.f, 0.f, 200.f, 200.f);
    auto halfProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.5f);

    ASSERT_EQ("[15.0%, 22.5%, 30.0%, 37.5%]", halfProgressInterpolated.toString());

    auto threeQuartersProgressInterpolated = interpolateBorderRadius(fromBorderRadius, toBorderRadius, bounds, 0.75f);

    ASSERT_EQ("[17.5%, 26.2%, 35.0%, 43.8%]", threeQuartersProgressInterpolated.toString());
}

} // namespace snap::drawing
