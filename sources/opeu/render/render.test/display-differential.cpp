// RenderDisplay against the native adapter, method by method and in composed scenes (display-differential.hpp says how
// a script is run and compared). Every case starts in graphics with the game's colours (the base and day palettes,
// colours 0-15 showing entries 0-15) over a busy screen, unless it is about the start state. Where the renderer
// differs by design, the case says so and draws so that the difference cannot show; docs/rendering.md lists them.
#include "display-differential.hpp"

#include "opeu/assets/palette.hpp"
#include "opeu/assets/planar.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace opeu::render::detail::display_differential_test {

using services::Area;
using services::Corners;
using services::Image;
using test::ExpectSame;
using test::Noise;
using test::Outcome;
using test::Shipped;
using test::StreamBytes;

// ---- helpers ------------------------------------------------------------------------------------------------------

[[nodiscard]] auto Levels(std::string_view const palette) -> std::vector<std::uint8_t> {
  return assets::FindPalette(palette).value().levels | std::ranges::to<std::vector>();
}

auto Push(Outcome& outcome, bool const drawn) -> void {
  outcome.push_back(drawn ? 1 : 0);
}

auto Push(Outcome& outcome, std::span<std::uint8_t const> const bytes) -> void {
  outcome.insert(outcome.end(), bytes.begin(), bytes.end());
}

// Graphics in the day colours over the whole game area painted with noise in every colour.
auto BusyScreen(services::Display& display) -> void {
  static auto const noise{ Noise(80uz * 400 * 4, 1) };
  display.EnterGraphics();
  display.ShowIdentityPalette();
  display.LoadPalette(0, Levels("main/day"));
  std::ignore = display.DrawColumns({ .x = 0, .y = 0, .width = 80, .height = 400 }, { .mode = 0, .bytes = noise });
}

// The same over a black screen.
auto QuietScreen(services::Display& display) -> void {
  display.EnterGraphics();
  display.ShowIdentityPalette();
  display.LoadPalette(0, Levels("main/day"));
}

auto const IMAGE{ Noise(2048, 7) };

// ---- the screen ---------------------------------------------------------------------------------------------------

TEST(DisplayDifferential, StartsEntersAndLeavesGraphicsAsTheCanvas) {
  ExpectSame([](services::Display&) { return Outcome{}; });
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    display.LeaveGraphics();
    return Outcome{};
  });
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    display.EnterGraphics();
    return Outcome{};
  });
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    display.Clear();
    return Outcome{};
  });
}

TEST(DisplayDifferential, RecoloursWhatIsDrawnWhenThePaletteChanges) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto const night{ Levels("main/night") };
    display.LoadPalette(8, std::span{ night }.subspan(24));
    return Outcome{};
  });
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto const title{ Levels("main/title") };
    auto const dusk{ Levels("main/dusk") };
    display.LoadPalette(3, std::span{ title }.first(15));
    display.LoadPalette(0x23, dusk); // 16 entries from 3: a count past 16 is cut at 16 - 3
    return Outcome{};
  });
  ExpectSame([](services::Display& display) { // the start colours, then the identity map over a drawn screen
    std::ignore = display.DrawColumns({ .x = 0, .y = 0, .width = 80, .height = 400 }, { .mode = 0, .bytes = Noise(80uz * 400 * 4, 3) });
    display.ShowIdentityPalette();
    return Outcome{};
  });
}

// ---- images -------------------------------------------------------------------------------------------------------

TEST(DisplayDifferential, DrawsColumnImagesInEveryMode) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto outcome{ Outcome{} };
    std::ranges::for_each(std::array<Word, 6>{ 0, 1, 2, 3, 4, 0x8000 }, [&](Word const mode) {
      Push(outcome, display.DrawColumns({ .x = static_cast<Word>(3 + mode % 7 * 10), .y = 20, .width = 4, .height = 9 }, { .mode = mode, .bytes = IMAGE }));
    });
    Push(outcome, display.DrawColumns({ .x = 79, .y = 0, .width = 2, .height = 1 }, { .mode = 0, .bytes = IMAGE }));  // past the right edge
    Push(outcome, display.DrawColumns({ .x = 0, .y = 395, .width = 2, .height = 6 }, { .mode = 0, .bytes = IMAGE })); // past the bottom
    Push(outcome, display.DrawColumns({ .x = 0, .y = 0, .width = 2, .height = 6 }, { .mode = 5, .bytes = IMAGE }));   // no such mode
    return outcome;
  });
}

TEST(DisplayDifferential, DrawsThreeByteColumnImagesInEveryMode) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto outcome{ Outcome{} };
    std::ranges::for_each(std::array<Word, 7>{ 0, 0x4000, 1, 2, 3, 4, 0x8000 }, [&](Word const mode) {
      Push(outcome, display.DrawThreeByteColumns({ .x = static_cast<Word>(5 + (mode & 7) * 9 + (mode >> 14) * 3), .y = 30, .width = 3, .height = 8 }, { .mode = mode, .bytes = IMAGE }));
    });
    Push(outcome, display.DrawThreeByteColumns({ .x = 0, .y = 0, .width = 3, .height = 8 }, { .mode = 0x4005, .bytes = IMAGE }));
    return outcome;
  });
}

TEST(DisplayDifferential, DrawsBitsWithEveryPen) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto outcome{ Outcome{} };
    std::ranges::for_each(std::array<Word, 8>{ 0x0005, 0x0409, 0x010C, 0x0203, 0x030A, 0x0705, 0x8402, 0x810E }, [&, x = Word{ 2 }](Word const mode) mutable {
      Push(outcome, display.DrawBits({ .x = x, .y = 50, .width = 6, .height = 10 }, { .mode = mode, .bytes = IMAGE }));
      x = static_cast<Word>(x + 9);
    });
    Push(outcome, display.DrawBits({ .x = 75, .y = 50, .width = 6, .height = 10 }, { .mode = 0x0105, .bytes = IMAGE }));
    return outcome;
  });
  ExpectSame([](services::Display& display) { // a pen code it does not know before any pen is chosen draws nothing
    BusyScreen(display);
    std::ignore = display.DrawBits({ .x = 2, .y = 50, .width = 6, .height = 10 }, { .mode = 0x0605, .bytes = IMAGE });
    return Outcome{};
  });
}

TEST(DisplayDifferential, DrawsBitsOnPaper) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    display.DrawBitsOnPaper({ .x = 2, .y = 50, .width = 6, .height = 10 }, { .mode = 0x030C, .bytes = IMAGE });
    display.DrawBitsOnPaper({ .x = 30, .y = 90, .width = 1, .height = 16 }, { .mode = 0x0F00, .bytes = IMAGE });
    display.DrawBitsOnPaper({ .x = 79, .y = 90, .width = 2, .height = 16 }, { .mode = 0x0F00, .bytes = IMAGE });
    return Outcome{};
  });
}

TEST(DisplayDifferential, DrawsShiftedBitsAtAnyPixel) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto outcome{ Outcome{} };
    std::ranges::for_each(std::array<Word, 6>{ 0x0104, 0x0009, 0x020B, 0x030E, 0x0406, 0x0507 }, [&, x = Word{ 13 }](Word const mode) mutable {
      Push(outcome, display.DrawShiftedBits({ .x = x, .y = 60, .width = 21, .height = 7 }, { .mode = mode, .bytes = IMAGE }));
      x = static_cast<Word>(x + 37);
    });
    Push(outcome, display.DrawShiftedBits({ .x = 630, .y = 60, .width = 11, .height = 7 }, { .mode = 0x0004, .bytes = IMAGE }));
    return outcome;
  });
}

// The Canvas gives a shifted image's transparent pixels after a row's first column the screen of that first column
// (a quirk the renderer drops, docs/rendering.md): over one colour the two agree.
TEST(DisplayDifferential, DrawsShiftedColumnsWithColourZeroTransparent) {
  ExpectSame([](services::Display& display) {
    QuietScreen(display);
    std::ignore = display.Box({ .x = 0, .y = 60, .width = 80, .height = 40 }, 9);
    display.DrawShiftedColumns({ .x = 37, .y = 70, .width = 3, .height = 5 }, IMAGE);
    display.DrawShiftedColumns({ .x = 200, .y = 64, .width = 6, .height = 30 }, std::span{ IMAGE }.subspan(100));
    return Outcome{};
  });
}

TEST(DisplayDifferential, SavesColumnsIntoTheGamesMemory) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto saved{ std::vector<std::uint8_t>(4uz * 4 * 6) };
    display.SaveColumns({ .x = 9, .y = 33, .width = 4, .height = 6 }, saved);
    return saved;
  });
}

TEST(DisplayDifferential, CopiesColumnRectangles) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    display.CopyColumns({ .x = 4, .y = 10, .width = 6, .height = 12 }, { .x = 9, .y = 14 });    // overlapping, down and right
    display.CopyColumns({ .x = 40, .y = 100, .width = 6, .height = 50 }, { .x = 38, .y = 90 }); // overlapping, up and left
    display.CopyColumns({ .x = 60, .y = 200, .width = 10, .height = 20 }, { .x = 2, .y = 300 });
    return Outcome{};
  });
}

TEST(DisplayDifferential, DecodesPicturesAndReadsTheirBytes) {
  // 8x3 in eight colours: two literal units; a literal unit and a copy of it; the row above copied down. (A copy that
  // reaches left of or above the picture reads the screen on the Canvas and black on the renderer: docs/rendering.md.)
  static auto const EIGHT{
    std::vector<std::uint8_t>{ 8, 0, 3, 0, 0x14, 0x21, 0x00, 0x35, 0x80, 0xC1, 0xFF }
  };
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto outcome{ Outcome{} };
    auto stream{ StreamBytes{ EIGHT } };
    Push(outcome, display.DecodePicture({ .x = 8, .y = 3 }, stream.Reader()));
    outcome.push_back(static_cast<std::uint8_t>(stream.Read()));
    auto bad{ StreamBytes{ { 0x81, 0x02, 1, 0 } } }; // 641 wide: refused after its width
    Push(outcome, display.DecodePicture({ .x = 8, .y = 3 }, bad.Reader()));
    outcome.push_back(static_cast<std::uint8_t>(bad.Read()));
    auto high{ StreamBytes{ EIGHT } }; // three rows from line 398
    Push(outcome, display.DecodePicture({ .x = 8, .y = 398 }, high.Reader()));
    outcome.push_back(static_cast<std::uint8_t>(high.Read()));
    return outcome;
  });
}

TEST(DisplayDifferential, DecodesShippedPictures) {
  ExpectSame([](services::Display& display) { // the title screen, sixteen colours
    QuietScreen(display);
    display.LoadPalette(0, Levels("main/title"));
    auto stream{ StreamBytes{ Shipped().FileFrom("SCESEL.DAT", 0) } };
    auto outcome{ Outcome{} };
    Push(outcome, display.DecodeMaskedPicture({ .x = 0, .y = 0 }, stream.Reader()));
    outcome.push_back(static_cast<std::uint8_t>(stream.Read() & 0xFF));
    return outcome;
  });
  ExpectSame([](services::Display& display) { // an event picture, eight colours, at a place not on a unit
    BusyScreen(display);
    auto stream{ StreamBytes{ Shipped().FileFrom("EVENT.DAT", 56uz * 512) } };
    auto outcome{ Outcome{} };
    Push(outcome, display.DecodePicture({ .x = 102, .y = 60 }, stream.Reader()));
    outcome.push_back(static_cast<std::uint8_t>(stream.Read() & 0xFF));
    return outcome;
  });
}

// ---- shapes -------------------------------------------------------------------------------------------------------

TEST(DisplayDifferential, DrawsBoxesAndHighlights) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto outcome{ Outcome{} };
    Push(outcome, display.Box({ .x = 2, .y = 3, .width = 5, .height = 7 }, 0x0B));
    Push(outcome, display.Box({ .x = 80, .y = 0, .width = 1, .height = 1 }, 1));
    Push(outcome, display.Highlight({ .x = 2, .y = 30, .width = 5, .height = 7 }, 0x0103));
    Push(outcome, display.Highlight({ .x = 12, .y = 30, .width = 5, .height = 7 }, 0x0206));
    Push(outcome, display.Highlight({ .x = 22, .y = 30, .width = 5, .height = 7 }, 0x0309));
    Push(outcome, display.Highlight({ .x = 32, .y = 30, .width = 5, .height = 7 }, 0x0003));
    Push(outcome, display.Highlight({ .x = 42, .y = 30, .width = 5, .height = 7 }, 0x040F));
    Push(outcome, display.DrawBits({ .x = 52, .y = 30, .width = 2, .height = 7 }, { .mode = 0x0700, .bytes = IMAGE })); // the highlight's pen
    return outcome;
  });
}

TEST(DisplayDifferential, DrawsLinesPlainDashedAndCombined) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto const line{ [&display](Corners const ends, Word const colour) { display.Line(ends, colour); } };
    line(
        {
            .from = { .x = 10,  .y = 20  },
              .to = { .x = 300, .y = 170 }
    },
        0xAA13); // XOR, dashed
    line(
        {
            .from = { .x = 300, .y = 30 },
              .to = { .x = 10,  .y = 60 }
    },
        0x000C); // set
    line(
        {
            .from = { .x = 5, .y = 390 },
              .to = { .x = 5, .y = 200 }
    },
        0xF00E); // set, dashed
    line(
        {
            .from = { .x = 600, .y = 10  },
              .to = { .x = 500, .y = 300 }
    },
        0x0025); // AND
    line(
        {
            .from = { .x = 20,  .y = 100 },
              .to = { .x = 620, .y = 101 }
    },
        0x0036); // OR
    line(
        {
            .from = { .x = 20,  .y = 110 },
              .to = { .x = 620, .y = 140 }
    },
        0xCC47); // OR on the dashes only
    line(
        {
            .from = { .x = 630, .y = 380 },
              .to = { .x = 700, .y = 420 }
    },
        0x5A09); // leaves the game area
    line(
        {
            .from = { .x = 77, .y = 77 },
              .to = { .x = 77, .y = 77 }
    },
        0x000F); // one point
    return Outcome{};
  });
}

TEST(DisplayDifferential, FillsInclusiveCorners) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    display.Fill(
        {
            .from = { .x = 5,   .y = 10 },
              .to = { .x = 221, .y = 31 }
    },
        6);
    display.Fill(
        {
            .from = { .x = 300, .y = 50 },
              .to = { .x = 302, .y = 50 }
    },
        0x1F);
    display.Fill(
        {
            .from = { .x = 9, .y = 300 },
              .to = { .x = 9, .y = 399 }
    },
        2);
    return Outcome{};
  });
}

// ---- battle cells and sprites ---------------------------------------------------------------------------------------

TEST(DisplayDifferential, SavesAndRestoresBattleCells) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto cell{ std::array<std::uint8_t, SYSLIB_CELL_BYTES>{} };
    auto outcome{ Outcome{} };
    display.SaveCell({ .x = 7, .y = 90 }, cell);
    Push(outcome, cell);
    display.SaveShiftedCell({ .x = 7, .y = 90 }, cell);
    Push(outcome, cell);
    display.RestoreCell({ .x = 10, .y = 100 }, services::CellImage{ std::span{ IMAGE }.first<SYSLIB_CELL_BYTES>() });
    display.RestoreShiftedCell({ .x = 20, .y = 50 }, services::CellImage{ std::span{ IMAGE }.subspan<100, SYSLIB_CELL_BYTES>() });
    display.CopyCellBlock({ .x = 10, .y = 100 }, { .x = 20, .y = 140 });
    return outcome;
  });
}

TEST(DisplayDifferential, ComposesCellBuffersAsTheLibrary) {
  ExpectSame([](services::Display& display) {
    auto cell{ std::array<std::uint8_t, SYSLIB_CELL_BYTES>{} };
    std::ranges::copy(std::span{ IMAGE }.subspan(100, cell.size()), cell.begin());
    display.OverlayCell(cell, std::span{ IMAGE }.subspan<300, SYSLIB_CELL_BYTES>());
    display.MaskCell(cell, std::span{ IMAGE }.subspan<900, services::CELL_MASK_BYTES>());
    display.DrawCellGlyph(cell, std::span{ IMAGE }.subspan<1200, services::CELL_GLYPH_LINES>(), { .x = 17, .y = 8 });
    display.DrawColouredCellGlyph(cell,
                                  std::span{
                                      IMAGE
    }
                                      .subspan<1300, services::CELL_GLYPH_LINES>(),
                                  { .at = { .x = 11, .y = 13 }, .colour = 0x0100 });
    return Outcome(cell.begin(), cell.end());
  });
}

TEST(DisplayDifferential, ComposesSpritesOverTheScreen) {
  ExpectSame([](services::Display& display) {
    BusyScreen(display);
    auto       buffer{ std::vector<std::uint8_t>(6uz * 26 * 4) };
    auto       outcome{ Outcome{} };
    auto const sprite{ [&](Word const flags) {
      return services::Sprite{ .and_mask = std::span{ IMAGE }.first(1024), .or_image = std::span{ IMAGE }.subspan(1024), .buffer = buffer, .flags = flags };
    } };
    display.ComposeSprite({ .x = 20, .y = 100, .width = 5, .height = 26 }, sprite(1));
    Push(outcome, buffer);
    display.ComposeSprite({ .x = 40, .y = 100, .width = 5, .height = 26 }, sprite(0));
    Push(outcome, buffer);
    display.ComposeShiftedSprite({ .x = 203, .y = 150, .width = 40, .height = 26 }, sprite(1));
    Push(outcome, buffer);
    display.ComposeShiftedSprite({ .x = 403, .y = 150, .width = 40, .height = 26 }, sprite(0)); // the buffer only
    Push(outcome, buffer);
    return outcome;
  });
}

}
