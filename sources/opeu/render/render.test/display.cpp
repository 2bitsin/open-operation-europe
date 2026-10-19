#include "headless.hpp"

#include "opeu/assets/palette.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/render/display.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

namespace opeu::render::detail::display_test {
namespace {

using test::ExpectGolden;
using test::OpenHeadless;
using test::Pixel;

constexpr auto AREA{
  Rect{ .x = 8, .y = 8, .width = 48, .height = 32 }
};
constexpr auto RED{
  Rgba{ .r = 0xD3, .g = 0x30, .b = 0x30 }
};
constexpr auto GREEN{
  Rgba{ .r = 0x41, .g = 0x92, .b = 0x41 }
};
constexpr auto HALF_TRANSPARENT{
  Rgba{ .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0x80 }
};
constexpr auto BLUE{
  Rgba{ .r = 0x10, .g = 0x41, .b = 0x92 }
};

// A 4x2 image: red, green, blue, white over black, transparent, half-transparent white, grey.
[[nodiscard]] auto Swatch() -> std::vector<Rgba> {
  return {
    RED, GREEN, BLUE, WHITE, BLACK, Rgba{ .a = 0 },
         HALF_TRANSPARENT, Rgba{ .r = 0x61, .g = 0x61, .b = 0x61 }
  };
}

[[nodiscard]] auto Image(std::vector<Rgba> const& pixels) -> assets::Texture {
  auto const bytes{ pixels | std::views::transform([](Rgba const c) { return std::array{ std::byte{ c.r }, std::byte{ c.g }, std::byte{ c.b }, std::byte{ c.a } }; }) | std::views::join
                    | std::ranges::to<std::vector>() };
  return assets::Texture{ 4, 2, bytes };
}

// The swatch as it is, scaled, flipped both ways, tinted half-transparent orange, and its middle scaled.
constexpr auto ORANGE_HALF{
  Rgba{ .r = 0xFF, .g = 0x80, .b = 0x00, .a = 0x80 }
};
constexpr auto MIDDLE{
  Rect{ .x = 1, .y = 0, .width = 2, .height = 2 }
};
// clang-format off
constexpr auto SWATCH_BLITS{ std::array{
  Blit{ .destination = Rect{ .x =  0, .y =  0, .width =  4, .height =  2 }, .source = std::nullopt, .flip = Flip::NONE, .modulation = WHITE       },
  Blit{ .destination = Rect{ .x =  8, .y =  0, .width = 32, .height = 16 }, .source = std::nullopt, .flip = Flip::NONE, .modulation = WHITE       },
  Blit{ .destination = Rect{ .x =  8, .y = 16, .width = 32, .height = 16 }, .source = std::nullopt, .flip = Flip::BOTH, .modulation = WHITE       },
  Blit{ .destination = Rect{ .x = 40, .y =  0, .width = 24, .height = 12 }, .source = std::nullopt, .flip = Flip::NONE, .modulation = ORANGE_HALF },
  Blit{ .destination = Rect{ .x = 40, .y = 16, .width = 16, .height = 16 }, .source = MIDDLE,       .flip = Flip::NONE, .modulation = WHITE       },
} };
// clang-format on

[[nodiscard]] auto Region(Display const& display) -> assets::Texture {
  return display.Read(Rect{ .x = 0, .y = 0, .width = 64, .height = 48 }).value();
}

TEST(Display, BeginsWithABlackFrame) {
  auto headless{ OpenHeadless() };
  headless.display.Begin();
  auto const frame{ headless.display.Read(LOGICAL_SCREEN).value() };
  EXPECT_EQ(frame.Width(), 640u);
  EXPECT_EQ(frame.Height(), 480u);
  EXPECT_TRUE(std::ranges::all_of(test::PixelsOf(frame), [](auto const pixel) { return pixel.front() == std::byte{ 0 } && pixel.back() == std::byte{ 0xFF }; }));
}

TEST(Display, FillsARectangle) {
  auto headless{ OpenHeadless() };
  headless.display.Begin();
  headless.display.Fill(AREA, RED);
  headless.display.Fill(Rect{ .x = 24, .y = 16, .width = 16, .height = 16 }, Rgba{ .r = 0x10, .g = 0x41, .b = 0x92, .a = 0x80 });
  ExpectGolden(Region(headless.display), "fill");
}

TEST(Display, DrawsLinesAndPoints) {
  auto headless{ OpenHeadless() };
  headless.display.Begin();
  headless.display.Line(Point{ .x = 2, .y = 2 }, Point{ .x = 60, .y = 40 }, GREEN);
  headless.display.Line(Point{ .x = 2, .y = 44 }, Point{ .x = 60, .y = 44 }, WHITE);
  headless.display.Line(Point{ .x = 32, .y = 2 }, Point{ .x = 32, .y = 30 }, RED);
  std::ranges::for_each(std::views::iota(0, 8), [&](int const n) { headless.display.Plot(Point{ .x = 4 + (2 * n), .y = 36 }, BLUE); });
  ExpectGolden(Region(headless.display), "line-and-point");
}

TEST(Display, BlitsWithScaleFlipAlphaAndColourModulation) {
  auto       headless{ OpenHeadless() };
  auto const swatch{ Texture::Upload(headless.display.Renderer(), Image(Swatch())).value() };
  headless.display.Begin();
  headless.display.Fill(Rect{ .x = 0, .y = 0, .width = 64, .height = 48 }, Rgba{ .r = 0x61, .g = 0x41, .b = 0x20 });
  std::ranges::for_each(SWATCH_BLITS, [&headless, &swatch](Blit const& blit) { headless.display.Draw(swatch, blit); });
  ExpectGolden(Region(headless.display), "blit");
}

TEST(Display, ReadsBackWhatWasDrawn) {
  auto       headless{ OpenHeadless() };
  auto const opaque{ [](Rgba const colour) { return Rgba{ .r = colour.r, .g = colour.g, .b = colour.b }; } };
  auto const image{ Image(Swatch() | std::views::transform(opaque) | std::ranges::to<std::vector>()) };
  auto const texture{ Texture::Upload(headless.display.Renderer(), image).value() };
  auto const area{
    Rect{ .x = 100, .y = 200, .width = 4, .height = 2 }
  };
  headless.display.Begin();
  headless.display.Draw(texture, Blit{ .destination = area });
  EXPECT_EQ(headless.display.Read(area).value(), image);
  EXPECT_EQ(Pixel(headless.display.Read(LOGICAL_SCREEN).value(), Point{ .x = 103, .y = 201 }), (Rgba{ .r = 0x61, .g = 0x61, .b = 0x61 }));
}

TEST(Display, RestoresASnapshotExactly) {
  auto  headless{ OpenHeadless() };
  auto& display{ headless.display };
  display.Begin();
  display.Fill(AREA, RED);
  display.Line(Point{ .x = 0, .y = 0 }, Point{ .x = 63, .y = 47 }, Rgba{ .r = 0xFF, .g = 0xFF, .b = 0x00, .a = 0x80 });
  auto const before{ Region(display) };
  auto const highlight{
    Rect{ .x = 16, .y = 12, .width = 24, .height = 16 }
  };
  auto const snapshot{ display.Take(highlight).value() };
  display.Fill(highlight, Rgba{ .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xA0 });
  EXPECT_NE(Region(display), before);
  display.Restore(snapshot);
  EXPECT_EQ(Region(display), before);
  ExpectGolden(before, "snapshot");
}

TEST(Display, CopiesARectangleAsItWasEvenOverlapping) {
  auto  headless{ OpenHeadless() };
  auto& display{ headless.display };
  display.Begin();
  display.Fill(Rect{ .x = 0, .y = 0, .width = 64, .height = 48 }, BLUE);
  display.Fill(Rect{ .x = 4, .y = 4, .width = 16, .height = 8 }, RED);
  display.Line(Point{ .x = 4, .y = 4 }, Point{ .x = 19, .y = 11 }, WHITE);
  auto const block{
    Rect{ .x = 4, .y = 4, .width = 16, .height = 8 }
  };
  auto const before{ display.Read(block).value() };
  ASSERT_TRUE(display.Copy(block, Point{ .x = 12, .y = 8 })); // overlaps its source: a scroll
  EXPECT_EQ(display.Read(Rect{ .x = 12, .y = 8, .width = 16, .height = 8 }).value(), before);
  ASSERT_TRUE(display.Copy(Rect{ .x = 0, .y = 0, .width = 32, .height = 24 }, Point{ .x = 32, .y = 24 })); // a block repeated
  ExpectGolden(Region(display), "copy");
}

TEST(Display, ShowsAnIndexedImageInAPalette) {
  auto       headless{ OpenHeadless() };
  auto const day{ assets::Colours(test::Needed(assets::FindPalette("main/day"), "main/day").levels) };
  auto const night{ assets::Colours(test::Needed(assets::FindPalette("main/night"), "main/night").levels) };
  // A 16x4 image of every colour index, and its lower half masked out.
  auto const indices{ std::views::iota(0uz, 64uz) | std::views::transform([](std::size_t const n) { return static_cast<std::uint8_t>(n % 16); }) | std::ranges::to<std::vector>() };
  auto const opaque{ std::views::iota(0uz, 64uz) | std::views::transform([](std::size_t const n) { return static_cast<std::uint8_t>(n < 32 ? 1 : 0); }) | std::ranges::to<std::vector>() };
  auto const image{
    assets::Image{ { .width = 16, .height = 4 }, indices, opaque }
  };
  auto const in_day{ Texture::Upload(headless.display.Renderer(), assets::ToTexture(image, day)).value() };
  auto const in_night{ Texture::Upload(headless.display.Renderer(), assets::ToTexture(image, night)).value() };
  headless.display.Begin();
  headless.display.Fill(Rect{ .x = 0, .y = 0, .width = 64, .height = 48 }, GREEN);
  headless.display.Draw(in_day, Blit{
                                    .destination = Rect{ .x = 0, .y = 0, .width = 64, .height = 16 }
  });
  headless.display.Draw(in_night, Blit{
                                      .destination = Rect{ .x = 0, .y = 24, .width = 64, .height = 16 }
  });
  ExpectGolden(Region(headless.display), "indexed-image");
}

TEST(Display, RefusesASnapshotOfTheWindow) {
  auto headless{ OpenHeadless() };
  headless.display.Present();
  EXPECT_FALSE(headless.display.Take(AREA).has_value());
}

TEST(Display, ComposesThroughALayer) {
  auto  headless{ OpenHeadless() };
  auto& display{ headless.display };
  auto  layer{ display.CreateLayer(Size{ .width = 16, .height = 16 }).value() };
  display.Begin();
  {
    auto const into{ display.Into(layer) };
    display.Fill(Rect{ .x = 0, .y = 0, .width = 16, .height = 8 }, GREEN);
    display.Line(Point{ .x = 0, .y = 15 }, Point{ .x = 15, .y = 0 }, WHITE);
  }
  display.Fill(Rect{ .x = 0, .y = 0, .width = 64, .height = 48 }, BLUE);
  auto const as_is{
    Rect{ .x = 4, .y = 4, .width = 16, .height = 16 }
  };
  auto const doubled{
    Rect{ .x = 24, .y = 4, .width = 32, .height = 32 }
  };
  display.Draw(layer, Blit{ .destination = as_is });
  display.Draw(layer, Blit{ .destination = doubled, .modulation = HALF_TRANSPARENT });
  ExpectGolden(Region(display), "layer");
}

TEST(Display, CapturesTheFrameTintedAsPresented) {
  auto  headless{ OpenHeadless() };
  auto& display{ headless.display };
  display.Begin();
  display.Fill(Rect{ .x = 0, .y = 0, .width = 32, .height = 48 }, WHITE);
  display.Fill(Rect{ .x = 32, .y = 0, .width = 32, .height = 48 }, RED);
  auto const half{ display.Capture(Dimmed(WHITE, 0x80)).value() };
  EXPECT_EQ(Pixel(half, Point{ .x = 0, .y = 0 }), (Rgba{ .r = 0x80, .g = 0x80, .b = 0x80 }));
  auto const evening{ display.Capture(Rgba{ .r = 0xFF, .g = 0xC0, .b = 0x80 }).value() };
  ExpectGolden(test::Crop(half, Size{ .width = 64, .height = 48 }), "fade-half");
  ExpectGolden(test::Crop(evening, Size{ .width = 64, .height = 48 }), "tint-evening");
  display.Present(Dimmed(WHITE, 0x80));
}

}

}
