#pragma once
// The game's palettes as a table: tables/palettes.json holds each palette's name, use and sixteen colours as the
// 4-bit DAC levels the game writes, blue, red and green (docs/asset-pack.md, "Tables").

#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/palette.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::palette_table {
using namespace std::string_literals;
using namespace std::string_view_literals;
inline constexpr auto PALETTE_TABLE{ "tables/palettes.json"sv };

// A palette read from the table: NamedPalette's fields, owning their text.
struct PaletteRecord {
  std::string name;
  Levels      levels{};
  std::string use;

  auto operator==(PaletteRecord const&) const -> bool = default;
};

[[nodiscard]] auto PaletteTableText(std::span<NamedPalette const> palettes) -> std::string;

[[nodiscard]] auto ReadPaletteTable(Bytes bytes) -> Packed<std::vector<PaletteRecord>>;

}

namespace opeu::assets {
using detail::palette_table::PALETTE_TABLE;
using detail::palette_table::PaletteRecord;
using detail::palette_table::PaletteTableText;
using detail::palette_table::ReadPaletteTable;
}
