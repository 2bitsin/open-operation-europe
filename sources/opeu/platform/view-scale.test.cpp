#include "opeu/platform/view-scale.hpp"

#include <gtest/gtest.h>

#include <optional>

namespace opeu::platform::detail::view_scale_test {

namespace {

constexpr auto FRAME{
  Size{ .width = 640, .height = 480 }
};
// A Retina laptop's usable bounds (1440x900 points, the menu bar off) and a 1080p monitor's.
constexpr auto LAPTOP{
  Size{ .width = 1440, .height = 809 }
};
constexpr auto MONITOR{
  Size{ .width = 1920, .height = 1050 }
};

[[nodiscard]] auto Desktop(float const density) -> View {
  return View{ .frame = FRAME, .density = density, .largest = 8 };
}

} // namespace

TEST(ViewScale, RoundsToTheNearestMultipleNotDown) {
  EXPECT_EQ(NearestScale(Desktop(1.0f), Size{ .width = 900, .height = 675 }), 1);   // 1.41
  EXPECT_EQ(NearestScale(Desktop(1.0f), Size{ .width = 1000, .height = 750 }), 2);  // 1.56
  EXPECT_EQ(NearestScale(Desktop(1.0f), Size{ .width = 1700, .height = 1275 }), 3); // 2.66
}

TEST(ViewScale, NeverGoesBelowOnce) {
  EXPECT_EQ(NearestScale(Desktop(1.0f), Size{ .width = 100, .height = 75 }), 1);
  EXPECT_EQ(NearestScale(Desktop(1.0f), Size{ .width = 0, .height = 0 }), 1);
}

TEST(ViewScale, NeverGoesAboveWhatTheScreenHolds) {
  auto const view{
    View{ .frame = FRAME, .density = 2.0f, .largest = 3 }
  };
  EXPECT_EQ(NearestScale(view, Size{ .width = 2880, .height = 2160 }), 3);
}

TEST(ViewScale, CountsMultiplesInDevicePixels) {
  // A 640x480-point window on a Retina display is 1280x960 pixels: 2x, and 2x is 640x480 points again.
  EXPECT_EQ(NearestScale(Desktop(2.0f), Size{ .width = 1280, .height = 960 }), 2);
  EXPECT_EQ(WindowSize(Desktop(2.0f), 2), (Size{ .width = 640, .height = 480 }));
  EXPECT_EQ(WindowSize(Desktop(2.0f), 3), (Size{ .width = 960, .height = 720 }));
  EXPECT_EQ(WindowSize(Desktop(1.0f), 2), (Size{ .width = 1280, .height = 960 }));
}

TEST(ViewScale, RoundsAFractionalDensityUpToWholeCoordinates) {
  EXPECT_EQ(WindowSize(Desktop(1.5f), 1), (Size{ .width = 427, .height = 320 }));
}

TEST(ViewScale, SnapsAWindowBetweenMultiples) {
  EXPECT_EQ(Snapped(Desktop(1.0f), Size{ .width = 1000, .height = 750 }), (Size{ .width = 1280, .height = 960 }));
  EXPECT_EQ(Snapped(Desktop(2.0f), Size{ .width = 1500, .height = 1125 }), (Size{ .width = 640, .height = 480 }));
  EXPECT_EQ(Snapped(Desktop(2.0f), Size{ .width = 2072, .height = 1554 }), (Size{ .width = 960, .height = 720 }));
}

TEST(ViewScale, LeavesAWindowAlreadyAtAMultiple) {
  EXPECT_EQ(Snapped(Desktop(1.0f), Size{ .width = 1280, .height = 960 }), std::nullopt);
  EXPECT_EQ(Snapped(Desktop(2.0f), Size{ .width = 1920, .height = 1440 }), std::nullopt);
  // 427x320 points at 1.5 are 641x480 pixels: the snap's own result, which must not snap again.
  EXPECT_EQ(Snapped(Desktop(1.5f), Size{ .width = 641, .height = 480 }), std::nullopt);
}

TEST(ViewScale, DefaultsToTheLargestMultipleTheUsableBoundsHold) {
  EXPECT_EQ(ViewOn(FRAME, Screen{ .usable = LAPTOP, .density = 2.0f }).largest, 3);  // 2784x1522 pixels
  EXPECT_EQ(ViewOn(FRAME, Screen{ .usable = LAPTOP, .density = 1.0f }).largest, 1);  // 1392x761
  EXPECT_EQ(ViewOn(FRAME, Screen{ .usable = MONITOR, .density = 1.0f }).largest, 2); // 1872x1002
  EXPECT_EQ(ViewOn(FRAME,
                   Screen{
                       .usable = Size{ .width = 800, .height = 600 },
                         .density = 1.0f
  })
                .largest,
            1);
}

TEST(ViewScale, FitsAtLeastOnceInAnyRoom) {
  EXPECT_EQ(FittingScale(Size{ .width = 10, .height = 10 }, FRAME), 1);
  EXPECT_EQ(FittingScale(Size{ .width = 1920, .height = 1440 }, FRAME), 3);
  EXPECT_EQ(FittingScale(Size{ .width = 1920, .height = 1440 }, Size{}), 1);
}

}
