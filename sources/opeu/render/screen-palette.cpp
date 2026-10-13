#include "opeu/render/screen-palette.hpp"

#include "opeu/assets/bytes.hpp"

#include <algorithm>
#include <ranges>

namespace opeu::render::detail::screen_palette {
namespace {

constexpr auto START_COLOUR_MAP{
  std::array<std::uint8_t, COLOUR_NUMBERS>{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F }
};

// Entry n's level of a primary: two thirds of full for bit `high`, one third for bit `low`.
[[nodiscard]] constexpr auto StartLevel(std::size_t const entry, std::size_t const high, std::size_t const low) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>((entry >> high & 1) * 0x2A + (entry >> low & 1) * 0x15);
}

// A 4-bit level as the library stores it: times four, in six bits.
[[nodiscard]] constexpr auto DacLevel(std::uint8_t const level) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>(level << 2 & 0x3F);
}

}

ScreenPalette::ScreenPalette() : _colour_map{ START_COLOUR_MAP } {
  std::ranges::transform(assets::Indices(_entries.size()), _entries.begin(),
                         [](std::size_t const entry) { return Entry{ .red = StartLevel(entry, 2, 5), .green = StartLevel(entry, 1, 4), .blue = StartLevel(entry, 0, 3) }; });
}

auto ScreenPalette::ShowIdentity() -> void {
  std::ranges::copy(std::views::iota(std::uint8_t{ 0 }, std::uint8_t{ COLOUR_NUMBERS }), _colour_map.begin());
}

auto ScreenPalette::Load(std::uint8_t const first, std::span<std::uint8_t const> const levels) -> void {
  auto const start{ first & 0x0FUZ };
  auto const count{ static_cast<std::uint8_t>(levels.size() / 3) };
  auto const entries{ count > COLOUR_NUMBERS ? COLOUR_NUMBERS - start : std::size_t{ count } };
  std::ranges::transform(assets::Records(std::as_bytes(levels), 3, entries), _entries.begin() + static_cast<std::ptrdiff_t>(start), [](assets::Bytes const level) {
    return Entry{ .red = DacLevel(assets::Byte(level, 1)), .green = DacLevel(assets::Byte(level, 2)), .blue = DacLevel(assets::Byte(level, 0)) };
  });
}

auto ScreenPalette::Colours() const -> assets::PaletteColours {
  auto colours{ assets::PaletteColours{} };
  std::ranges::transform(_colour_map, colours.begin(), [this](std::uint8_t const entry) {
    auto const& levels{ _entries.at(entry) };
    return assets::Colour{ .r = Shown(levels.red), .g = Shown(levels.green), .b = Shown(levels.blue) };
  });
  return colours;
}

auto ScreenPalette::EntryLevels() const -> std::array<std::uint8_t, PALETTE_ENTRIES * 3> {
  auto levels{ std::array<std::uint8_t, PALETTE_ENTRIES * 3>{} };
  std::ranges::transform(assets::Grid(_entries.size(), 3), levels.begin(), [this](auto const place) {
    auto const& entry{ _entries.at(place.first) };
    return Shown(std::array{ entry.red, entry.green, entry.blue }.at(place.second));
  });
  return levels;
}

}
