#include "opeu/assets/catalogue.hpp"

#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/maps.hpp"
#include "opeu/assets/palette.hpp"
#include "opeu/assets/picture-stream.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/assets/runs.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::catalogue {

namespace {

constexpr auto BLOCK{ std::uint32_t{ 512 } };

// Where each equipment silhouette starts in ARMS.SIL, in units of four bytes.
constexpr std::array<unsigned, 159> SILHOUETTE_AT{
  0,     121,   344,   723,   1116,  1527,  1941,  2318,  2439,  2642,  2849,  3060,  3337,  3448,  3588,  3723,  3865,  4041,  4219,  4404,  4590,  4781,  4958,  5141,  5331,  5526,  5755,
  5962,  6167,  6392,  6588,  6745,  6910,  7080,  7252,  7396,  7548,  7742,  7975,  8185,  8361,  8584,  8724,  8863,  9052,  9227,  9440,  9597,  9691,  9798,  9913,  10058, 10175, 10289,
  10438, 10597, 10708, 10831, 11009, 11385, 11766, 11887, 12269, 12645, 12912, 13042, 13263, 13482, 13670, 13793, 13924, 14080, 14232, 14450, 14832, 14971, 15133, 15335, 15476, 15637, 15789,
  15953, 16127, 16206, 16350, 16524, 16698, 16824, 17210, 17598, 17792, 17951, 18060, 18194, 18477, 18665, 18826, 18978, 19146, 19347, 19543, 19765, 19976, 20151, 20307, 20450, 20555, 20679,
  20808, 20947, 21095, 21270, 21416, 21565, 21718, 21874, 22024, 22192, 22350, 22441, 22826, 23004, 23183, 23378, 23499, 23656, 23865, 23963, 24086, 24235, 24379, 24786, 25165, 25546, 25695,
  25841, 26108, 26355, 26623, 26841, 27017, 27199, 27423, 27649, 27851, 28040, 28236, 28410, 28625, 28786, 28946, 29054, 29164, 29277, 29439, 29595, 29730, 29890, 30081,
};

struct Placed {
  std::uint32_t    at;
  std::string_view name;
  std::string_view use;
};

constexpr auto EVENT_PICTURES{ std::array<Placed, 32>{ {
    { 0, "00", "" },
    { 15, "01", "" },
    { 30, "02", "" },
    { 42, "03", "" },
    { 56, "04", "a commander lost" },
    { 72, "05", "" },
    { 91, "06", "" },
    { 104, "07", "" },
    { 115, "08", "the daily report" },
    { 136, "09", "a visit" },
    { 150, "10", "" },
    { 162, "11", "the war goes on" },
    { 178, "12", "a commander lost" },
    { 186, "13", "a spy" },
    { 202, "14", "a spy" },
    { 215, "15", "" },
    { 231, "16", "a hurricane; partisans" },
    { 250, "17", "a parachute drop" },
    { 259, "18", "" },
    { 271, "19", "a special operation" },
    { 288, "20", "" },
    { 300, "21", "a mission to kill" },
    { 318, "22", "" },
    { 342, "23", "" },
    { 358, "24", "" },
    { 379, "25", "the personnel menu" },
    { 401, "26", "" },
    { 418, "27", "a bridge built" },
    { 436, "28", "" },
    { 455, "29", "" },
    { 471, "30", "" },
    { 486, "31", "" },
} } };

constexpr auto SELECTION_PICTURES{ std::array<Placed, 7>{ {
    { 0, "screen", "the title and scenario-selection screen" },
    { 29, "scenario-1", "France, 10 May 1940" },
    { 44, "scenario-2", "Africa, 26 May 1942" },
    { 51, "scenario-3", "Kursk, 5 July 1943" },
    { 65, "scenario-4", "D-Day, 6 June 1944" },
    { 76, "scenario-5", "the Bulge, 16 December 1944" },
    { 91, "scenario-6", "Berlin, 16 April 1945" },
} } };

constexpr auto OUTCOME_PICTURES{ std::array<Placed, 10>{ {
    { 0, "medal-0", "" },
    { 3, "medal-1", "" },
    { 8, "medal-2", "" },
    { 12, "medal-3", "" },
    { 17, "medal-4", "" },
    { 22, "medal-5", "" },
    { 27, "axis-wins", "the outcome screen" },
    { 80, "axis-loses", "the outcome screen" },
    { 147, "allies-wins", "the outcome screen" },
    { 217, "allies-loses", "the outcome screen" },
} } };

constexpr auto FRAME_PICTURES{ std::array<Placed, 9>{ {
    { 0, "top-bar", "the top bar of the turn" },
    { 1, "map-frame", "the frame around the map" },
    { 7, "message-panels", "the message panels right of the map" },
    { 13, "strip-1", "scenario 1's title strip" },
    { 18, "strip-2", "scenario 2's title strip" },
    { 23, "strip-3", "scenario 3's title strip" },
    { 28, "strip-4", "scenario 4's title strip" },
    { 33, "strip-5", "scenario 5's title strip" },
    { 38, "strip-6", "scenario 6's title strip" },
} } };

constexpr auto EPILOGUE_PICTURES{ std::array<Placed, 7>{ {
    { 0x00000, "0", "the epilogue" },
    { 0x03D20, "1", "the epilogue" },
    { 0x0B4B1, "2", "the epilogue" },
    { 0x0D03B, "3", "the epilogue" },
    { 0x0F2BC, "4", "" },
    { 0x10C78, "5", "the epilogue" },
    { 0x13829, "6", "the epilogue's END" },
} } };

struct OpeningPicture {
  std::uint32_t    at;
  std::string_view palette;
};

// Each opening picture with the palette its scene shows it in.
constexpr auto OPENING_PICTURES{ std::array<OpeningPicture, 30>{ {
    { 0x00000, "open/002" }, { 0x0042D, "open/002" }, { 0x007F0, "open/002" }, { 0x00A81, "open/003" }, { 0x093F7, "open/004" }, { 0x121A0, "open/005" },
    { 0x27B7D, "open/011" }, { 0x28343, "open/030" }, { 0x28C1B, "open/030" }, { 0x2955F, "open/031" }, { 0x37110, "open/004" }, { 0x399B9, "open/003" },
    { 0x3E98B, "open/033" }, { 0x40BD5, "open/033" }, { 0x421D5, "open/033" }, { 0x43202, "open/033" }, { 0x43FD2, "open/036" }, { 0x46848, "open/037" },
    { 0x47428, "open/037" }, { 0x487FB, "open/037" }, { 0x48CE9, "open/037" }, { 0x4D847, "open/037" }, { 0x54D6D, "open/042" }, { 0x5B770, "open/046" },
    { 0x5B9E3, "open/046" }, { 0x5D486, "open/046" }, { 0x5EDDE, "open/047" }, { 0x5F135, "open/047" }, { 0x5FF59, "open/111" }, { 0x63A2C, "open/111" },
} } };

class Builder {
public:
  auto Add(Asset asset) -> void {
    _assets.push_back(std::move(asset));
  }
  [[nodiscard]] auto Take() -> std::vector<Asset> {
    return std::move(_assets);
  }

private:
  std::vector<Asset> _assets;
};

auto Stream(std::string id, std::string_view const file, std::uint32_t const at, Layout const layout, std::string_view const palette, std::string_view const use) -> Asset {
  return Asset{ .id              = std::move(id),
                .file            = file,
                .group           = "pictures",
                .at              = at,
                .mask_at         = 0,
                .layout          = layout,
                .width_bytes     = 0,
                .height          = 0,
                .role            = Role::PICTURE,
                .palette         = palette,
                .ink             = 0,
                .ink_where_clear = false,
                .side_coloured   = false,
                .hour_coloured   = false,
                .animation       = {},
                .frame           = 0,
                .use             = std::string{ use } };
}

auto AddPictures(Builder& out) -> void {
  for (auto const& p : EVENT_PICTURES) {
    out.Add(Stream(std::format("event/{}", p.name), "EVENT.DAT", p.at * BLOCK, Layout::STREAM_8, "main/day", p.use));
  }
  for (auto const& p : SELECTION_PICTURES) {
    out.Add(Stream(std::format("title/{}", p.name), "SCESEL.DAT", p.at * BLOCK, Layout::STREAM_16, "main/title", p.use));
  }
  for (auto const& p : OUTCOME_PICTURES) {
    out.Add(Stream(std::format("outcome/{}", p.name), "SCEEND.DAT", p.at * BLOCK, Layout::STREAM_8, "main/map", p.use));
  }
  for (auto const& p : FRAME_PICTURES) {
    out.Add(Stream(std::format("frame/{}", p.name), "MGAMEN.DAT", p.at * BLOCK, Layout::STREAM_16, "main/day", p.use));
  }
  for (auto const& p : EPILOGUE_PICTURES) {
    out.Add(Stream(std::format("epilogue/{}", p.name), "ENDGRP.NPK", p.at, Layout::STREAM_16, "main/epilogue", p.use));
  }
  for (auto const n : Indices(OPENING_PICTURES.size())) {
    auto const& p{ OPENING_PICTURES[n] };
    out.Add(Stream(std::format("opening/{:02}", n), "OPENGRP.NPK", p.at, Layout::STREAM_16, p.palette, "the opening"));
  }
}

auto Raw(std::string id, std::string_view const file, std::string_view const group, std::uint32_t const at, Layout const layout, std::size_t const width_bytes, std::size_t const height,
         Role const role) -> Asset {
  return Asset{ .id              = std::move(id),
                .file            = file,
                .group           = group,
                .at              = at,
                .mask_at         = 0,
                .layout          = layout,
                .width_bytes     = static_cast<unsigned>(width_bytes),
                .height          = static_cast<unsigned>(height),
                .role            = role,
                .palette         = "main/day",
                .ink             = 0,
                .ink_where_clear = false,
                .side_coloured   = false,
                .hour_coloured   = false,
                .animation       = {},
                .frame           = 0,
                .use             = {} };
}

auto AddTiles(Builder& out) -> void {
  for (auto const map : Indices(MAP_COUNT)) {
    for (auto const tile : Indices(TILES_PER_MAP)) {
      auto asset{ Raw(std::format("overview-tile/map-{}/{:02}", map, tile), "BIGMAP.DAT", "tiles", static_cast<std::uint32_t>(map * 0xC00 + tile * 32), Layout::PLANES_4, 1, 8, Role::TILE) };
      asset.hour_coloured = true;
      asset.use           = "the strategic overview, through MAPPNO.DAT";
      out.Add(std::move(asset));
    }
  }
  for (auto const map : Indices(MAP_COUNT)) {
    for (auto const tile : Indices(TILES_PER_MAP)) {
      auto asset{ Raw(std::format("view-tile/map-{}/{:02}", map, tile), "MIDMAP.DAT", "tiles", static_cast<std::uint32_t>(map * 0x6C00 + tile * 0x120), Layout::PLANES_4, 3, 24, Role::TILE) };
      asset.hour_coloured = true;
      asset.use           = "the tactical map view, through MAPPNO.DAT";
      out.Add(std::move(asset));
    }
  }
}

auto TerrainUse(std::size_t const record) -> std::string {
  constexpr auto PARTS{
    std::array<std::string_view, 4>{ "the diagonal cell", "the cell below", "its own cell", "the cell to the right" }
  };
  if (record < 68) return std::format("battle terrain object {}, the part over {}", record / 4, PARTS[record % 4]);
  if (record < 72) return std::format("battle ground {}", record - 68);
  return std::format("battle overlay {} (river, road or bridge)", record - 72);
}

auto AddBattleSprites(Builder& out) -> void {
  for (auto const& [file, set, use] : std::array<std::array<std::string_view, 3>, 2>{
           { { "HEXFONT.DAT", "summer", "" }, { "HEXFONTS.DAT", "snow", " (the Bulge, scenario 5)" } }
  }) {
    for (auto const record : Indices(95)) {
      auto asset{ Raw(std::format("battle-terrain/{}/{:02}", set, record), file, "battle", static_cast<std::uint32_t>(record * CELL_BYTES), Layout::CELL, 5, 26, Role::SPRITE) };
      asset.hour_coloured = true;
      asset.use           = TerrainUse(record) + std::string{ use };
      out.Add(std::move(asset));
    }
  }
  for (auto const record : Indices(52)) {
    auto const at{ static_cast<std::uint32_t>(record * CELL_BYTES) };
    if (record < 44) {
      auto asset{ Raw(std::format("battle-unit/class-{:02}/facing-{}", record / 4, record % 4), "HEXUNIT.DAT", "battle", at, Layout::CELL, 5, 26, Role::SPRITE) };
      asset.side_coloured = true;
      asset.use           = "a battle unit icon";
      out.Add(std::move(asset));
      continue;
    }
    auto const size{ record < 48 ? std::string_view{ "large" } : std::string_view{ "small" } };
    auto       asset{ Raw(std::format("battle-blast/{}/{}", size, record % 4), "HEXUNIT.DAT", "battle", at, Layout::CELL, 5, 26, Role::SPRITE) };
    asset.animation = std::format("battle-blast/{}", size);
    asset.frame     = static_cast<unsigned>(record % 4);
    asset.use       = record < 48 ? "a hit in battle" : "a miss in battle";
    out.Add(std::move(asset));
  }
  for (auto const& [name, at] : std::array<std::pair<std::string_view, std::uint32_t>, 2>{
           { { "nw-se", 0x8408 }, { "ne-sw", 0x8534 } }
  }) {
    auto asset{ Raw(std::format("battle-border/{}", name), "HEXUNIT.DAT", "battle", at, Layout::MASKED_SPRITE, 3, 20, Role::SPRITE) };
    asset.mask_at = at + 240;
    asset.use     = "the battle field's diagonal border";
    out.Add(std::move(asset));
  }
}

auto AddPortraitsAndSilhouettes(Builder& out) -> void {
  for (auto const n : Indices(120)) {
    auto asset{ Raw(std::format("portrait/{:03}", n), "FACE.DAT", "portraits", static_cast<std::uint32_t>(n * 0x780), Layout::PLANES_3, 8, 80, Role::PORTRAIT) };
    asset.use = n == 118 ? "the spy" : n == 119 ? "Sister Angelique" : "a commander";
    out.Add(std::move(asset));
  }
  for (auto const n : Indices(SILHOUETTE_AT.size())) {
    auto asset{ Raw(std::format("silhouette/{:03}", n), "ARMS.SIL", "silhouettes", static_cast<std::uint32_t>(SILHOUETTE_AT[n]) * 4, Layout::RUNS_BITMAP, 24, 80, Role::SILHOUETTE) };
    asset.ink             = 7;
    asset.ink_where_clear = true;
    asset.use             = "an equipment type's panel";
    out.Add(std::move(asset));
  }
}

auto Bitmap(std::string id, std::string_view const file, std::string_view const group, std::uint32_t const at, std::size_t const width_bytes, std::size_t const height, Role const role,
            std::uint8_t const ink, bool const where_clear, std::string_view const use) -> Asset {
  auto asset{ Raw(std::move(id), file, group, at, Layout::BITMAP, width_bytes, height, role) };
  asset.ink             = ink;
  asset.ink_where_clear = where_clear;
  asset.use             = use;
  return asset;
}

auto AddScreens(Builder& out) -> void {
  struct Part {
    std::string_view name;
    std::uint32_t    at;
    std::size_t      width_bytes, height;
    std::string_view use;
  };
  for (auto const& p : std::array<Part, 6>{
           { { "number-dialog", 0x0000, 18, 96, "the number entry dialog" },
            { "yes", 0x1B00, 4, 16, "YES, released" },
            { "yes-pressed", 0x1C00, 4, 16, "YES, pressed" },
            { "no", 0x1D00, 4, 16, "NO, released" },
            { "no-pressed", 0x1E00, 4, 16, "NO, pressed" },
            { "supply-panel", 0x1F00, 32, 120, "the supply and transfer panel" } }
  }) {
    auto asset{ Raw(std::format("dialog/{}", p.name), "MSGRP.DAT", "dialogs", p.at, Layout::PLANES_4, p.width_bytes, p.height, Role::SPRITE) };
    asset.use = p.use;
    out.Add(std::move(asset));
  }
  for (auto const& p : std::array<Part, 4>{
           { { "koei", 0x0000, 26, 112, "the KOEI logo" },
            { "copyright-1992", 0x0B60, 32, 104, "" },
            { "presents", 0x1860, 26, 24, "the opening's KOEI PRESENTS" },
            { "copyright-1994", 0x1AD1, 40, 40, "the copyright line" } }
  }) {
    auto asset{ Bitmap(std::format("logo/{}", p.name), "KOEILOGO.FNT", "logos", p.at, p.width_bytes, p.height, Role::CAPTION, p.name == "koei" ? 2 : 7, false, p.use) };
    asset.palette = "main/title";
    out.Add(std::move(asset));
  }
}

auto AddMasks(Builder& out) -> void {
  for (auto const k : Indices(6)) {
    auto asset{ Bitmap(std::format("opening-blast-mask/{}", k), "MOPENGRP.DAT", "masks", static_cast<std::uint32_t>(k * 0x11E0), 26, 176, Role::MASK, 0, true, "the opening's bomb explosions") };
    asset.animation = "opening-blast-mask";
    asset.frame     = static_cast<unsigned>(k);
    out.Add(std::move(asset));
  }
  constexpr auto SHELLS{ std::array<std::array<unsigned, 3>, 6>{ { { 0x6B40, 8, 40 }, { 0x6C80, 6, 16 }, { 0x6CE0, 4, 16 }, { 0x6D20, 1, 8 }, { 0x6D28, 2, 8 }, { 0x6D38, 1, 8 } } } };
  for (auto const k : Indices(SHELLS.size())) {
    auto asset{ Bitmap(std::format("opening-shell/{}", k), "MOPENGRP.DAT", "masks", SHELLS[k][0], SHELLS[k][1], SHELLS[k][2], Role::SPRITE, 7, false, "the opening's artillery shell in flight") };
    asset.palette   = "open/047";
    asset.animation = "opening-shell";
    asset.frame     = static_cast<unsigned>(k);
    out.Add(std::move(asset));
  }
  for (auto const k : Indices(8)) {
    auto asset{ Bitmap(std::format("ending-aircraft-mask/{}", k), "MENDGRP.DAT", "masks", static_cast<std::uint32_t>(k * 2048), 32, 64, Role::MASK, 0, true, "") };
    asset.animation = "ending-aircraft-mask";
    asset.frame     = static_cast<unsigned>(k);
    out.Add(std::move(asset));
  }
}

auto AddGlyphs(Builder& out) -> void {
  for (auto const n : Indices(ENDING_GLYPHS)) {
    out.Add(Bitmap(std::format("ending-glyph/{:03}", n), "EMSG.16P", "glyphs", static_cast<std::uint32_t>(n * GLYPH_BYTES), 2, GLYPH_HEIGHT, Role::GLYPH, 7, false,
                   std::format("the ending's text, code 0x{:04X}", EndingGlyphCodes()[n])));
  }
  for (auto const n : Indices(GAME_GLYPHS)) {
    out.Add(Bitmap(std::format("game-glyph/{:04}", n), "MSG.16P", "glyphs", static_cast<std::uint32_t>(n * GLYPH_BYTES), 2, GLYPH_HEIGHT, Role::GLYPH, 7, false, ""));
  }
}

// A one-bit asset as its shape drawn in the ink, everything else transparent.
auto Inked(Image const& bits, Asset const& asset) -> Image {
  auto opaque{ bits.Pixels() | std::views::transform([&](std::uint8_t const bit) { return static_cast<std::uint8_t>((bit != 0) != asset.ink_where_clear); }) | std::ranges::to<std::vector>() };
  auto pixels{ opaque | std::views::transform([&](std::uint8_t const drawn) { return drawn != 0 ? asset.ink : std::uint8_t{ 0 }; }) | std::ranges::to<std::vector>() };
  return Image{ bits.Width(), bits.Height(), std::move(pixels), std::move(opaque) };
}

auto Region(Bytes const file, Asset const& asset, std::size_t const planes) -> Decoded<Bytes> {
  return Need(file, asset.at, std::size_t{ asset.width_bytes } * asset.height * planes);
}

}

auto DecodeAsset(Bytes const file, Asset const& asset) -> Decoded<Image> {
  if (asset.at >= file.size()) return std::unexpected{ DecodeError::OUT_OF_RANGE };
  switch (asset.layout) {
  case Layout::PLANES_4:
  case Layout::PLANES_3: {
    auto const planes{ asset.layout == Layout::PLANES_4 ? 4uz : 3uz };
    return Region(file, asset, planes).and_then([&](Bytes const bytes) { return DecodeInterleaved(bytes, asset.width_bytes, asset.height, planes); });
  }
  case Layout::BITMAP:
    return Region(file, asset, 1).and_then([&](Bytes const bytes) { return DecodeBitmap(bytes, asset.width_bytes, asset.height); }).transform([&](Image const& bits) { return Inked(bits, asset); });
  case Layout::RUNS_BITMAP:
    return ExpandRuns(file.subspan(asset.at), std::size_t{ asset.width_bytes } * asset.height)
        .and_then([&](Expanded const& expanded) { return DecodeBitmap(expanded.bytes, asset.width_bytes, asset.height); })
        .transform([&](Image const& bits) { return Inked(bits, asset); });
  case Layout::CELL:          return Need(file, asset.at, CELL_BYTES).and_then(DecodeCell);
  case Layout::MASKED_SPRITE: {
    auto const size{ std::size_t{ asset.width_bytes } * asset.height };
    auto const image{ Need(file, asset.at, size * 4) };
    auto const mask{ Need(file, asset.mask_at, size) };
    if (!image || !mask) return std::unexpected{ DecodeError::TRUNCATED };
    return DecodeMaskedSprite(*image, *mask, asset.width_bytes, asset.height);
  }
  case Layout::STREAM_8:
  case Layout::STREAM_16:
    return DecodePicture(file.subspan(asset.at), asset.layout == Layout::STREAM_8 ? Colours::EIGHT : Colours::SIXTEEN).transform([](Picture const& picture) { return picture.image; });
  }
  return std::unexpected{ DecodeError::OUT_OF_RANGE };
}

auto SideZeroColours(Image const& image) -> Image {
  return image.Recoloured([](std::uint8_t const index) { return (index & 0xFC) | (index & 1) << 1 | (index >> 1 & 1); });
}

auto Spelling(Layout const layout) noexcept -> std::string_view {
  switch (layout) {
  case Layout::PLANES_4:      return "planes-4";
  case Layout::PLANES_3:      return "planes-3";
  case Layout::BITMAP:        return "bitmap";
  case Layout::RUNS_BITMAP:   return "runs-bitmap";
  case Layout::CELL:          return "cell";
  case Layout::MASKED_SPRITE: return "masked-sprite";
  case Layout::STREAM_8:      return "stream-8";
  case Layout::STREAM_16:     return "stream-16";
  }
  return "unknown";
}

auto Spelling(Role const role) noexcept -> std::string_view {
  switch (role) {
  case Role::PICTURE:    return "picture";
  case Role::TILE:       return "tile";
  case Role::SPRITE:     return "sprite";
  case Role::PORTRAIT:   return "portrait";
  case Role::SILHOUETTE: return "silhouette";
  case Role::MASK:       return "mask";
  case Role::CAPTION:    return "caption";
  case Role::GLYPH:      return "glyph";
  }
  return "unknown";
}

Catalogue::Catalogue(Source const& source) : _source{ source } {
  auto builder{ Builder{} };
  AddPictures(builder);
  AddTiles(builder);
  AddBattleSprites(builder);
  AddPortraitsAndSilhouettes(builder);
  AddScreens(builder);
  AddMasks(builder);
  AddGlyphs(builder);
  _assets = builder.Take();
}

auto Catalogue::Find(std::string_view const id) const -> std::optional<Asset> {
  auto const found{ std::ranges::find(_assets, id, &Asset::id) };
  if (found == _assets.end()) return std::nullopt;
  return *found;
}

auto Catalogue::Decode(Asset const& asset) const -> Decoded<Image> {
  return _source.get().File(asset.file).and_then([&](Bytes const file) { return DecodeAsset(file, asset); });
}

auto Catalogue::DecodeTexture(Asset const& asset) const -> Decoded<Texture> {
  auto const palette{ FindPalette(asset.palette) };
  if (!palette) return std::unexpected{ DecodeError::OUT_OF_RANGE };
  return Decode(asset).transform([&](Image const& image) { return ToTexture(image, *palette); });
}

}
