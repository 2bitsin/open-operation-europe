// The opening's buffer compositing and screen routines that the game's library lacks: a screen save that writes
// through a segment:offset destination (wrapping inside its segment), far copy, OR compose, AND mask, and the
// dissolve.
//
// Every count is a 16-bit loop count, as the original's LOOP took it: a count of 0 runs 65536 times. Every offset wraps
// inside its 64 KiB.
//
// The dissolve replaces, where its mask bit is clear, every colour bit with colour bit 3 of the screen pixel, so those
// pixels turn 0 or 15 until a later step reveals them.
#include "open-services.hpp"

#include "canvas.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <ranges>

namespace opeu::services::detail::open_services {
namespace {
constexpr int           DISSOLVE_BIT{ 3 };       // the colour bit the dissolve keeps from the screen
constexpr std::uint32_t LINE_COLUMNS{ 80 };      // 8-pixel columns per screen line
constexpr std::uint32_t DISSOLVE_ROWS{ 0x120 };  // the dissolve's lines
constexpr std::uint32_t DISSOLVE_PAIRS{ 0x13 };  // the dissolve's column pairs per line (38 columns)
constexpr std::uint32_t DISSOLVE_STRIDE{ 0x98 }; // the saved picture's bytes per line
constexpr auto          PLANES{ std::views::iota(0, 4) };

// A 16-bit offset or column number, wrapped.
constexpr auto Wrap(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value);
}

// A 16-bit loop count's iterations: 0 runs 65536 times.
constexpr auto LoopCount(std::uint16_t count) -> std::uint32_t {
  return count == 0 ? std::uint32_t{ 0x10000 } : std::uint32_t{ count };
}

constexpr auto Steps(std::uint16_t count) {
  return std::views::iota(std::uint32_t{ 0 }, LoopCount(count));
}

// Column x (8 pixels) of game line y as a 16-bit column number, 80 per line.
auto ScreenColumn(Point at) -> std::uint16_t {
  return Wrap((Wrap(at.y + std::uint32_t{ canvas::GAME_TOP }) * LINE_COLUMNS) + at.x);
}

auto ColumnOffset(std::uint16_t column) -> Offset {
  return Offset{ column } * 8;
}

auto SaveRectangleOf(Canvas const& canvas, Area const area, FarBytes const destination) -> void {
  auto const    start{ ScreenColumn({ .x = area.x, .y = area.y }) };
  std::uint16_t near{ destination.offset };
  std::ranges::for_each(Steps(area.height), [&](std::uint32_t const row) {
    std::ranges::for_each(Steps(area.width), [&](std::uint32_t const column) {
      auto const column_at{ Wrap(start + (row * LINE_COLUMNS) + column) };
      std::ranges::for_each(PLANES, [&](int const plane) {
        destination.segment[near] = canvas.PlaneBits(ColumnOffset(column_at), plane);
        near                      = Wrap(near + 1u);
      });
    });
  });
}

// One step of the dissolve: where it lands, the saved picture and the step's 16-bit mask.
struct Dissolve {
  Point         at;
  FarBytes      source;
  std::uint16_t pattern{};
};

auto DissolveOn(Canvas& canvas, Dissolve const& step) -> void {
  auto const start{ ScreenColumn(step.at) };
  auto const source{ step.source };
  std::ranges::for_each(std::views::iota(std::uint32_t{ 0 }, DISSOLVE_ROWS), [&](std::uint32_t const row) {
    auto const shown{ std::rotl(step.pattern, static_cast<int>(row % 16)) }; // the mask rotates left by one per row
    std::ranges::for_each(PLANES, [&](int const plane) {
      std::ranges::for_each(std::views::iota(std::uint32_t{ 0 }, DISSOLVE_PAIRS), [&](std::uint32_t const pair) {
        auto const screen{ Wrap(start + (row * LINE_COLUMNS) + (pair * 2)) };
        auto const near{ Wrap(source.offset + (row * DISSOLVE_STRIDE) + static_cast<std::uint32_t>(plane) + (pair * 8)) };
        auto const picture{ static_cast<std::uint16_t>((source.segment[near] << 8) | source.segment[Wrap(near + 4u)]) };
        auto const kept{ static_cast<std::uint16_t>((canvas.PlaneBits(ColumnOffset(screen), DISSOLVE_BIT) << 8) | canvas.PlaneBits(ColumnOffset(Wrap(screen + 1u)), DISSOLVE_BIT)) };
        auto const bits{ static_cast<std::uint16_t>((picture & shown) | (kept & static_cast<std::uint16_t>(~shown))) };
        canvas.SetPlaneBits(ColumnOffset(screen), plane, static_cast<std::uint8_t>(bits >> 8));
        canvas.SetPlaneBits(ColumnOffset(Wrap(screen + 1u)), plane, static_cast<std::uint8_t>(bits));
      });
    });
  });
}

} // namespace

auto SaveScreenRectangle(Area const area, FarBytes const destination) -> void {
  SaveRectangleOf(Screen(), area, destination);
}

auto CopyFarWords(FarBytes const source, FarBytes const destination, std::uint16_t const count) -> void {
  std::ranges::for_each(std::views::iota(std::uint32_t{ 0 }, std::uint32_t{ count } & ~1u),
                        [&](std::uint32_t const step) { destination.segment[Wrap(destination.offset + step)] = source.segment[Wrap(source.offset + step)]; });
}

auto OrIntoBuffer(SavedRectangle const target, FarBytes const sprite) -> void {
  auto const bytes{ std::uint64_t{ LoopCount(target.height) } * LoopCount(Wrap(target.width * 2u)) * 2 };
  std::ranges::for_each(std::views::iota(std::uint64_t{ 0 }, bytes), [&](std::uint64_t const step) {
    target.bytes.segment[Wrap(static_cast<std::uint32_t>(target.bytes.offset + step))] |= sprite.segment[Wrap(static_cast<std::uint32_t>(sprite.offset + step))];
  });
}

auto AndWithMask(SavedRectangle const target, FarBytes const mask) -> void {
  auto const pairs{ std::uint64_t{ LoopCount(target.height) } * LoopCount(static_cast<std::uint16_t>(target.width >> 1)) };
  std::ranges::for_each(std::views::iota(std::uint64_t{ 0 }, pairs), [&](std::uint64_t const pair) {
    auto const mask_near{ Wrap(static_cast<std::uint32_t>(mask.offset + (pair * 2))) };
    auto const even{ mask.segment[mask_near] };
    auto const odd{ mask.segment[Wrap(mask_near + 1u)] };
    auto const first{ Wrap(static_cast<std::uint32_t>(target.bytes.offset + (pair * 8))) };
    std::ranges::for_each(std::views::iota(std::uint32_t{ 0 }, std::uint32_t{ 8 }), [&](std::uint32_t const byte) { target.bytes.segment[Wrap(first + byte)] &= (byte & 1u) != 0 ? odd : even; });
  });
}

auto DissolveStep(Point const at, FarBytes const source, std::uint16_t const pattern) -> void {
  DissolveOn(Screen(), { .at = at, .source = source, .pattern = pattern });
}
} // namespace opeu::services::detail::open_services
