// RenderDisplay against the native adapter in the screens the game composes: a map view with an hour change, a dialog
// with a portrait and text, a battle cell with a unit and a tracer. Each frame is also a golden of the classic pack
// (render.test/goldens/classic/display-*.png), and the shipped images drawn opaque in them must name their pack image in
// the source layer: the bytes the game hands over are recognised and the classic sample holds every one.
#include "display-differential.hpp"

#include "opeu/assets/palette.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace opeu::render::detail::display_scenes_test {
namespace {

using test::CLASSIC;
using test::ExpectGolden;
using test::ExpectSame;
using test::Outcome;
using test::RenderedRun;
using test::Shipped;

[[nodiscard]] auto Levels(std::string_view const palette) -> std::vector<std::uint8_t> {
  return test::Needed(assets::FindPalette(palette), palette).levels | std::ranges::to<std::vector>();
}

auto GameColours(services::Display& display) -> void {
  display.EnterGraphics();
  display.ShowIdentityPalette();
  display.LoadPalette(0, Levels("main/day"));
}

// `at_least` shipped images named their pack image, and the pack lacked none.
auto ExpectFromThePack(RenderedRun const& run, std::size_t const at_least) -> void {
  EXPECT_GE(run.statistics.recognised, at_least);
  EXPECT_EQ(run.statistics.unpacked, 0U);
}

// ---- a map view ---------------------------------------------------------------------------------------------------

constexpr auto TILE_BYTES{ 3uz * 24 * 4 };

// The view's 14x14 cells of 24x24 from column 1, line 16, tiles of the classic sample, a cursor XORed twice over one
// cell (gone again), a ruler under the view.
auto MapView(services::Display& display) -> void {
  GameColours(display);
  auto const tiles{ std::views::iota(10, 19) | std::views::transform([](int const n) { return Shipped().Record(std::format("view-tile/map-2/{}", n), TILE_BYTES); }) | std::ranges::to<std::vector>() };
  std::ranges::for_each(assets::Grid(14, 14), [&](auto const cell) {
    auto const& tile{ tiles.at(((cell.first * 5) + (cell.second * 3)) % tiles.size()) };
    auto const  at{
      services::Area{ .x = static_cast<Word>(1 + (cell.second * 3)), .y = static_cast<Word>(16 + (cell.first * 24)), .width = 3, .height = 24 }
    };
    std::ignore = display.DrawColumns(at, { .mode = 0, .bytes = tile });
  });
  std::ignore = display.Highlight({ .x = 13, .y = 88, .width = 3, .height = 24 }, 0x010F);
  std::ignore = display.Highlight({ .x = 13, .y = 88, .width = 3, .height = 24 }, 0x010F);
  display.Line(
      {
          .from = { .x = 8,   .y = 356 },
            .to = { .x = 343, .y = 356 }
  },
      0x000F);
}

TEST(DisplayScenes, AMapViewByDayAndAfterNightfall) {
  auto const day{ ExpectSame([](services::Display& display) {
    MapView(display);
    return Outcome{};
  }) };
  ExpectFromThePack(day, 14uz * 14);
  ExpectGolden(day.frame, CLASSIC, "display-map-day");
  auto const night{ ExpectSame([](services::Display& display) {
    MapView(display);
    auto const levels{ Levels("main/night") };
    display.LoadPalette(8, std::span{ levels }.subspan(24));
    return Outcome{};
  }) };
  ExpectGolden(night.frame, CLASSIC, "display-map-night");
}

// ---- a dialog with text -------------------------------------------------------------------------------------------

constexpr auto PORTRAIT_BYTES{ 8uz * 80 * 3 };
constexpr auto GLYPH_BYTES{ 16uz };

// The glyph bytes of the text font for each character.
[[nodiscard]] auto Glyphs(std::string_view const text) -> std::vector<std::vector<std::uint8_t>> {
  return text | std::views::transform([](char const c) { return Shipped().Record(std::format("text-glyph/{:03}", static_cast<int>(c)), GLYPH_BYTES); }) | std::ranges::to<std::vector>();
}

// A line of text in the mode it is drawn in.
struct Written {
  std::string_view text;
  Word             mode{};
};

auto Write(services::Display& display, Written const written, services::Point const at) -> void {
  std::ranges::for_each(std::views::zip(Glyphs(written.text), std::views::iota(Word{ 0 })), [&](auto const entry) {
    auto const& [glyph, n]{ entry };
    display.DrawBitsOnPaper({ .x = static_cast<Word>(at.x + n), .y = at.y, .width = 1, .height = 16 }, { .mode = written.mode, .bytes = glyph });
  });
}

auto Dialog(services::Display& display) -> void {
  GameColours(display);
  std::ignore = display.Box({ .x = 10, .y = 60, .width = 50, .height = 120 }, 1);
  display.Fill(
      {
          .from = { .x = 84,  .y = 64  },
            .to = { .x = 475, .y = 175 }
  },
      0);
  display.Line(
      {
          .from = { .x = 84,  .y = 64 },
            .to = { .x = 475, .y = 64 }
  },
      0x0007);
  std::ignore = display.DrawThreeByteColumns({ .x = 11, .y = 80, .width = 8, .height = 80 }, { .mode = 0, .bytes = Shipped().Record("portrait/118", PORTRAIT_BYTES) });
  Write(display, Written{ .text = "General, the bridge", .mode = 0x000F }, { .x = 21, .y = 80 });
  Write(display, Written{ .text = "at Arnhem is held.", .mode = 0x000F }, { .x = 21, .y = 96 });
  Write(display, Written{ .text = "Attack", .mode = 0x000E }, { .x = 21, .y = 128 });
  Write(display, Written{ .text = "Wait", .mode = 0x000E }, { .x = 21, .y = 144 });
  auto const title{ Glyphs("REPORT") };
  std::ranges::for_each(std::views::zip(title, std::views::iota(Word{ 0 })), [&](auto const entry) {
    auto const& [glyph, n]{ entry };
    std::ignore = display.DrawBits({ .x = static_cast<Word>(30 + n), .y = 62, .width = 1, .height = 16 }, { .mode = 0x040B, .bytes = glyph });
  });
  std::ignore = display.Highlight({ .x = 21, .y = 128, .width = 6, .height = 16 }, 0x0107); // the menu row chosen: one XOR
}

TEST(DisplayScenes, ADialogWithAPortraitAndText) {
  auto const run{ ExpectSame([](services::Display& display) {
    Dialog(display);
    return Outcome{};
  }) };
  ExpectFromThePack(run, 1 + 19 + 18 + 6 + 4); // the title's set-pen glyphs keep the box under their clear bits: raw
  ExpectGolden(run.frame, CLASSIC, "display-dialog");
}

// ---- a battle cell ------------------------------------------------------------------------------------------------

constexpr auto CELL_RECORD{ 650uz };

// A field, a unit composed over it in a cell buffer (the screen saved, the unit's mask ANDed, its image ORed, the
// buffer restored), and a tracer XORed from it, `tracers` times.
auto BattleCell(services::Display& display, int const tracers) -> Outcome {
  GameColours(display);
  std::ignore = display.Box({ .x = 0, .y = 40, .width = 80, .height = 300 }, 2);
  display.Fill(
      {
          .from = { .x = 0,   .y = 140 },
            .to = { .x = 639, .y = 190 }
  },
      10);
  auto const unit{ Shipped().Record("battle-unit/class-03/facing-1", CELL_RECORD) };
  auto       cell{ std::array<std::uint8_t, SYSLIB_CELL_BYTES>{} };
  display.SaveCell({ .x = 20, .y = 150 }, cell);
  display.MaskCell(cell, std::span{ unit }.subspan<SYSLIB_CELL_BYTES, services::CELL_MASK_BYTES>());
  display.OverlayCell(cell, std::span{ unit }.first<SYSLIB_CELL_BYTES>());
  display.RestoreShiftedCell({ .x = 20, .y = 150 }, cell);
  std::ranges::for_each(std::views::iota(0, tracers), [&display](int) {
    display.Line(
        {
            .from = { .x = 180, .y = 163 },
              .to = { .x = 470, .y = 90  }
    },
        0x001C);
  });
  return Outcome{};
}

TEST(DisplayScenes, ABattleCellWithAUnitAndATracer) {
  auto const traced{ ExpectSame([](services::Display& display) { return BattleCell(display, 1); }) };
  ExpectGolden(traced.frame, CLASSIC, "display-battle");
  auto const erased{ ExpectSame([](services::Display& display) { return BattleCell(display, 2); }) };
  auto const untraced{ ExpectSame([](services::Display& display) { return BattleCell(display, 0); }) };
  EXPECT_EQ(erased.frame, untraced.frame); // the tracer drawn twice is gone
}

// What the pump presents today, from either display: the frame as palette entries and the entries' levels.
struct Indexed {
  std::vector<std::uint8_t>                frame = std::vector<std::uint8_t>(FRAME_PIXELS);
  std::array<std::uint8_t, PALETTE_LEVELS> palette{};

  [[nodiscard]] auto Frame() -> std::span<std::uint8_t, FRAME_PIXELS> {
    return std::span<std::uint8_t, FRAME_PIXELS>{ frame.data(), FRAME_PIXELS };
  }
};

TEST(DisplayScenes, FillThePumpsIndexedFrameAsTheCanvas) {
  auto native{ Indexed{} };
  std::ignore = test::RunNative([](services::Display& display) {
    Dialog(display);
    return Outcome{};
  });
  std::ignore = VgalibRender(native.Frame(), native.palette);
  auto rendered_display{ test::Rendered{} };
  std::ignore = rendered_display.Play([](services::Display& display) {
    Dialog(display);
    return Outcome{};
  });
  auto rendered{ Indexed{} };
  rendered_display.Screen().RenderIndexed(rendered.Frame(), rendered.palette);
  EXPECT_EQ(native.frame, rendered.frame);
  EXPECT_EQ(native.palette, rendered.palette);
}

}

}
