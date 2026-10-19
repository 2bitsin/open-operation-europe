#include "opeu/assets/catalogue.hpp"

#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/maps.hpp"
#include "opeu/assets/palette.hpp"
#include "opeu/assets/picture-stream.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/assets/program-glyphs.hpp"
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

// Where a record lies in its file and how it is laid out.
struct Record {
  std::string_view file;
  std::string_view group;
  std::uint32_t    at{};
  Layout           layout{};
  std::size_t      width_bytes{};
  std::size_t      height{};
  Role             role{};
};

// A file of picture streams: the ids' prefix, the file, the unit its offsets count, its layout and the palette the
// game shows them in.
struct Streams {
  std::string_view prefix;
  std::string_view file;
  std::uint32_t    unit{};
  Layout           layout{};
  std::string_view palette;
};

// The colour a one-bit record's shape is drawn in, and whether the shape is its clear bits (an AND mask).
struct Ink {
  std::uint8_t colour{};
  bool         where_clear{};
};

auto Raw(std::string id, Record const& record) -> Asset {
  return Asset{ .id              = std::move(id),
                .file            = record.file,
                .group           = record.group,
                .at              = record.at,
                .mask_at         = 0,
                .layout          = record.layout,
                .width_bytes     = static_cast<unsigned>(record.width_bytes),
                .width           = 0,
                .height          = static_cast<unsigned>(record.height),
                .role            = record.role,
                .palette         = "main/day",
                .ink             = 0,
                .ink_where_clear = false,
                .side_coloured   = false,
                .hour_coloured   = false,
                .animation       = {},
                .frame           = 0,
                .use             = {} };
}

// The asset with the use the game puts it to.
auto Used(Asset asset, std::string_view const use) -> Asset {
  asset.use = use;
  return asset;
}

// A picture stream, which states its own size.
auto Stream(std::string id, Placed const& placed, Streams const& streams) -> Asset {
  auto asset{ Raw(std::move(id), Record{ .file = streams.file, .group = "pictures", .at = placed.at * streams.unit, .layout = streams.layout, .width_bytes = 0, .height = 0, .role = Role::PICTURE }) };
  asset.palette = streams.palette;
  return Used(std::move(asset), placed.use);
}

// Every picture of a file.
auto AddStreams(Builder& out, std::span<Placed const> const pictures, Streams const& streams) -> void {
  for (auto const& p : pictures) out.Add(Stream(std::format("{}/{}", streams.prefix, p.name), p, streams));
}

auto AddPictures(Builder& out) -> void {
  AddStreams(out, EVENT_PICTURES, Streams{ .prefix = "event", .file = "EVENT.DAT", .unit = BLOCK, .layout = Layout::STREAM_8, .palette = "main/day" });
  AddStreams(out, SELECTION_PICTURES, Streams{ .prefix = "title", .file = "SCESEL.DAT", .unit = BLOCK, .layout = Layout::STREAM_16, .palette = "main/title" });
  AddStreams(out, OUTCOME_PICTURES, Streams{ .prefix = "outcome", .file = "SCEEND.DAT", .unit = BLOCK, .layout = Layout::STREAM_8, .palette = "main/map" });
  AddStreams(out, FRAME_PICTURES, Streams{ .prefix = "frame", .file = "MGAMEN.DAT", .unit = BLOCK, .layout = Layout::STREAM_16, .palette = "main/day" });
  AddStreams(out, EPILOGUE_PICTURES, Streams{ .prefix = "epilogue", .file = "ENDGRP.NPK", .unit = 1, .layout = Layout::STREAM_16, .palette = "main/epilogue" });
  for (auto const [n, p] : std::views::zip(Indices(OPENING_PICTURES.size()), OPENING_PICTURES)) {
    auto const streams{
      Streams{ .prefix = "opening", .file = "OPENGRP.NPK", .unit = 1, .layout = Layout::STREAM_16, .palette = p.palette }
    };
    out.Add(Stream(std::format("opening/{:02}", n), Placed{ .at = p.at, .name = {}, .use = "the opening" }, streams));
  }
}

// A record whose colours 8..15 follow the hour.
auto Tile(std::string id, Record const& record) -> Asset {
  auto asset{ Raw(std::move(id), record) };
  asset.hour_coloured = true;
  return asset;
}

auto AddTiles(Builder& out) -> void {
  for (auto const [map, tile] : Grid(MAP_COUNT, TILES_PER_MAP)) {
    auto const at{ static_cast<std::uint32_t>((map * 0xC00) + (tile * 32)) };
    auto const record{
      Record{ .file = "BIGMAP.DAT", .group = "tiles", .at = at, .layout = Layout::PLANES_4, .width_bytes = 1, .height = 8, .role = Role::TILE }
    };
    out.Add(Used(Tile(std::format("overview-tile/map-{}/{:02}", map, tile), record), "the strategic overview, through MAPPNO.DAT"));
  }
  for (auto const [map, tile] : Grid(MAP_COUNT, TILES_PER_MAP)) {
    auto const at{ static_cast<std::uint32_t>((map * 0x6C00) + (tile * 0x120)) };
    auto const record{
      Record{ .file = "MIDMAP.DAT", .group = "tiles", .at = at, .layout = Layout::PLANES_4, .width_bytes = 3, .height = 24, .role = Role::TILE }
    };
    out.Add(Used(Tile(std::format("view-tile/map-{}/{:02}", map, tile), record), "the tactical map view, through MAPPNO.DAT"));
  }
}

auto TerrainUse(std::size_t const record) -> std::string {
  constexpr auto PARTS{
    std::array<std::string_view, 4>{ "the diagonal cell", "the cell below", "its own cell", "the cell to the right" }
  };
  if (record < 68) return std::format("battle terrain object {}, the part over {}", record / 4, PARTS.at(record % 4));
  if (record < 72) return std::format("battle ground {}", record - 68);
  return std::format("battle overlay {} (river, road or bridge)", record - 72);
}

// The 40x26 cell sprite `record` of a battle file.
auto Cell(std::string_view const file, std::size_t const record) -> Record {
  return Record{ .file = file, .group = "battle", .at = static_cast<std::uint32_t>(record * CELL_BYTES), .layout = Layout::CELL, .width_bytes = 5, .height = 26, .role = Role::SPRITE };
}

auto AddBattleTerrain(Builder& out) -> void {
  for (auto const& [file, set, use] : std::array<std::array<std::string_view, 3>, 2>{
           { { "HEXFONT.DAT", "summer", "" }, { "HEXFONTS.DAT", "snow", " (the Bulge, scenario 5)" } }
  }) {
    for (auto const record : Indices(95)) out.Add(Used(Tile(std::format("battle-terrain/{}/{:02}", set, record), Cell(file, record)), TerrainUse(record) + std::string{ use }));
  }
}

auto UnitIcon(std::size_t const record) -> Asset {
  auto asset{ Raw(std::format("battle-unit/class-{:02}/facing-{}", record / 4, record % 4), Cell("HEXUNIT.DAT", record)) };
  asset.side_coloured = true;
  return Used(std::move(asset), "a battle unit icon");
}

auto Blast(std::size_t const record) -> Asset {
  auto const large{ record < 48 };
  auto const size{ large ? std::string_view{ "large" } : std::string_view{ "small" } };
  auto       asset{ Raw(std::format("battle-blast/{}/{}", size, record % 4), Cell("HEXUNIT.DAT", record)) };
  asset.animation = std::format("battle-blast/{}", size);
  asset.frame     = static_cast<unsigned>(record % 4);
  return Used(std::move(asset), large ? "a hit in battle" : "a miss in battle");
}

auto AddBattleSprites(Builder& out) -> void {
  AddBattleTerrain(out);
  for (auto const record : Indices(52)) out.Add(record < 44 ? UnitIcon(record) : Blast(record));
  for (auto const& [name, at] : std::array<std::pair<std::string_view, std::uint32_t>, 2>{
           { { "nw-se", 0x8408 }, { "ne-sw", 0x8534 } }
  }) {
    auto asset{ Raw(std::format("battle-border/{}", name),
                    Record{ .file = "HEXUNIT.DAT", .group = "battle", .at = at, .layout = Layout::MASKED_SPRITE, .width_bytes = 3, .height = 20, .role = Role::SPRITE }) };
    asset.mask_at = at + 240;
    out.Add(Used(std::move(asset), "the battle field's diagonal border"));
  }
}

auto PortraitUse(std::size_t const n) -> std::string_view {
  if (n == 118) return "the spy";
  if (n == 119) return "Sister Angelique";
  return "a commander";
}

auto WithInk(Asset asset, Ink const ink) -> Asset {
  asset.ink             = ink.colour;
  asset.ink_where_clear = ink.where_clear;
  return asset;
}

auto AddPortraitsAndSilhouettes(Builder& out) -> void {
  for (auto const n : Indices(120)) {
    auto const record{
      Record{ .file = "FACE.DAT", .group = "portraits", .at = static_cast<std::uint32_t>(n * 0x780), .layout = Layout::PLANES_3, .width_bytes = 8, .height = 80, .role = Role::PORTRAIT }
    };
    out.Add(Used(Raw(std::format("portrait/{:03}", n), record), PortraitUse(n)));
  }
  for (auto const [n, at] : std::views::zip(Indices(SILHOUETTE_AT.size()), SILHOUETTE_AT)) {
    auto const record{
      Record{ .file = "ARMS.SIL", .group = "silhouettes", .at = at * 4, .layout = Layout::RUNS_BITMAP, .width_bytes = 24, .height = 80, .role = Role::SILHOUETTE }
    };
    out.Add(Used(WithInk(Raw(std::format("silhouette/{:03}", n), record), Ink{ .colour = 7, .where_clear = true }), "an equipment type's panel"));
  }
}

// A one-bit bitmap drawn in an ink.
auto Bitmap(std::string id, Record record, Ink const ink) -> Asset {
  record.layout = Layout::BITMAP;
  return WithInk(Raw(std::move(id), record), ink);
}

// A sequence's frame.
auto Framed(Asset asset, std::string_view const animation, std::size_t const frame) -> Asset {
  asset.animation = animation;
  asset.frame     = static_cast<unsigned>(frame);
  return asset;
}

struct Part {
  std::string_view name;
  std::uint32_t    at;
  std::size_t      width_bytes, height;
  std::string_view use;
};

auto AddDialogs(Builder& out) -> void {
  for (auto const& p : std::array<Part, 6>{
           { { "number-dialog", 0x0000, 18, 96, "the number entry dialog" },
            { "yes", 0x1B00, 4, 16, "YES, released" },
            { "yes-pressed", 0x1C00, 4, 16, "YES, pressed" },
            { "no", 0x1D00, 4, 16, "NO, released" },
            { "no-pressed", 0x1E00, 4, 16, "NO, pressed" },
            { "supply-panel", 0x1F00, 32, 120, "the supply and transfer panel" } }
  }) {
    auto const record{
      Record{ .file = "MSGRP.DAT", .group = "dialogs", .at = p.at, .layout = Layout::PLANES_4, .width_bytes = p.width_bytes, .height = p.height, .role = Role::SPRITE }
    };
    out.Add(Used(Raw(std::format("dialog/{}", p.name), record), p.use));
  }
}

auto AddLogos(Builder& out) -> void {
  for (auto const& p : std::array<Part, 4>{
           { { "koei", 0x0000, 26, 112, "the KOEI logo" },
            { "copyright-1992", 0x0B60, 32, 104, "" },
            { "presents", 0x1860, 26, 24, "the opening's KOEI PRESENTS" },
            { "copyright-1994", 0x1AD1, 40, 40, "the copyright line" } }
  }) {
    auto const record{
      Record{ .file = "KOEILOGO.FNT", .group = "logos", .at = p.at, .layout = Layout::BITMAP, .width_bytes = p.width_bytes, .height = p.height, .role = Role::CAPTION }
    };
    auto asset{ Bitmap(std::format("logo/{}", p.name), record, Ink{ .colour = static_cast<std::uint8_t>(p.name == "koei" ? 2 : 7), .where_clear = false }) };
    asset.palette = "main/title";
    out.Add(Used(std::move(asset), p.use));
  }
}

auto AddMasks(Builder& out) -> void {
  constexpr auto MASK_INK{
    Ink{ .colour = 0, .where_clear = true }
  };
  for (auto const k : Indices(6)) {
    auto const record{
      Record{ .file = "MOPENGRP.DAT", .group = "masks", .at = static_cast<std::uint32_t>(k * 0x11E0), .layout = Layout::BITMAP, .width_bytes = 26, .height = 176, .role = Role::MASK }
    };
    out.Add(Used(Framed(Bitmap(std::format("opening-blast-mask/{}", k), record, MASK_INK), "opening-blast-mask", k), "the opening's bomb explosions"));
  }
  constexpr auto SHELLS{ std::array<std::array<unsigned, 3>, 6>{ { { 0x6B40, 8, 40 }, { 0x6C80, 6, 16 }, { 0x6CE0, 4, 16 }, { 0x6D20, 1, 8 }, { 0x6D28, 2, 8 }, { 0x6D38, 1, 8 } } } };
  for (auto const [k, shell] : std::views::zip(Indices(SHELLS.size()), SHELLS)) {
    auto const [at, width_bytes, height]{ shell };
    auto const record{
      Record{ .file = "MOPENGRP.DAT", .group = "masks", .at = at, .layout = Layout::BITMAP, .width_bytes = width_bytes, .height = height, .role = Role::SPRITE }
    };
    auto asset{ Framed(Bitmap(std::format("opening-shell/{}", k), record, Ink{ .colour = 7, .where_clear = false }), "opening-shell", k) };
    asset.palette = "open/047";
    out.Add(Used(std::move(asset), "the opening's artillery shell in flight"));
  }
  for (auto const k : Indices(8)) {
    auto const record{
      Record{ .file = "MENDGRP.DAT", .group = "masks", .at = static_cast<std::uint32_t>(k * 2048), .layout = Layout::BITMAP, .width_bytes = 32, .height = 64, .role = Role::MASK }
    };
    out.Add(Framed(Bitmap(std::format("ending-aircraft-mask/{}", k), record, MASK_INK), "ending-aircraft-mask", k));
  }
}

// A glyph of `height` one-byte rows at `at` in a font file or a program table.
auto GlyphAt(std::string_view const file, std::size_t const at, std::size_t const height) -> Record {
  return Record{ .file = file, .group = "glyphs", .at = static_cast<std::uint32_t>(at), .layout = Layout::BITMAP, .width_bytes = 1, .height = height, .role = Role::GLYPH };
}

constexpr auto GLYPH_INK{
  Ink{ .colour = 7, .where_clear = false }
};

auto AddGlyphs(Builder& out) -> void {
  for (auto const [n, code] : std::views::zip(Indices(ENDING_GLYPHS), EndingGlyphCodes())) {
    auto record{ GlyphAt("EMSG.16P", n * GLYPH_BYTES, GLYPH_HEIGHT) };
    record.width_bytes = 2;
    out.Add(Used(Bitmap(std::format("ending-glyph/{:03}", n), record, GLYPH_INK), std::format("the ending's text, code 0x{:04X}", code)));
  }
  for (auto const n : Indices(GAME_GLYPHS)) {
    auto record{ GlyphAt("MSG.16P", n * GLYPH_BYTES, GLYPH_HEIGHT) };
    record.width_bytes = 2;
    out.Add(Bitmap(std::format("game-glyph/{:04}", n), record, GLYPH_INK));
  }
}

auto DigitUse(std::size_t const digit, std::string_view const where) -> std::string {
  return digit < 10 ? std::format("{}, {}", where, digit) : std::format("{}, the blank that clears a digit's cell", where);
}

auto AddProgramGlyphs(Builder& out) -> void {
  for (auto const n : Indices(TEXT_GLYPHS)) {
    auto const code{ FIRST_TEXT_CODE + n };
    auto const record{ GlyphAt(PROGRAM_TEXT_FONT, n * TEXT_GLYPH_HEIGHT, TEXT_GLYPH_HEIGHT) };
    out.Add(Used(Bitmap(std::format("text-glyph/{:03}", code), record, GLYPH_INK), std::format("the game's text, code 0x{:02X}", code)));
  }
  for (auto const n : Indices(DIGIT_GLYPHS)) {
    auto digit{ Bitmap(std::format("digit-glyph/{:02}", n), GlyphAt(PROGRAM_DIGITS, n * DIGIT_SIZE, DIGIT_SIZE), GLYPH_INK) };
    digit.width = DIGIT_SIZE;
    out.Add(Used(std::move(digit), DigitUse(n, "the map scales' and battle panels' digits")));
  }
  for (auto const n : Indices(DIGIT_GLYPHS)) {
    auto const record{ GlyphAt(PROGRAM_SIDEWAYS_DIGITS, n * SIDEWAYS_DIGIT_HEIGHT, SIDEWAYS_DIGIT_HEIGHT) };
    out.Add(Used(Bitmap(std::format("sideways-digit-glyph/{:02}", n), record, GLYPH_INK), DigitUse(n, "the vertical map scale's digits")));
  }
  for (auto const [n, use] : std::views::zip(Indices(MARKERS), std::array{ std::string_view{ "an objective on the overview" }, std::string_view{ "the overview's flashing cell" } })) {
    out.Add(Used(Bitmap(std::format("marker-glyph/{}", n), GlyphAt(PROGRAM_MARKERS, n * MARKER_SIZE, MARKER_SIZE), GLYPH_INK), use));
  }
}

// The left `width` pixels of every row (all of them when `width` is 0).
auto Narrowed(Image const& image, std::size_t const width) -> Image {
  if (width == 0 || width >= image.Width()) return image;
  auto const left{ [&image, width](std::span<std::uint8_t const> const values) {
    return Indices(image.Height()) | std::views::transform([values, &image, width](std::size_t const row) { return values.subspan(row * image.Width(), width); }) | std::views::join
           | std::ranges::to<std::vector>();
  } };
  return Image{
    { .width = width, .height = image.Height() },
    left(image.Pixels()), image.HasMask() ? left(image.Opaque()) : std::vector<std::uint8_t>{}
  };
}

// A one-bit asset as its shape drawn in the ink, everything else transparent.
auto Inked(Image const& bits, Asset const& asset) -> Image {
  auto opaque{ bits.Pixels() | std::views::transform([&](std::uint8_t const bit) { return static_cast<std::uint8_t>((bit != 0) != asset.ink_where_clear); }) | std::ranges::to<std::vector>() };
  auto pixels{ opaque | std::views::transform([&](std::uint8_t const drawn) { return drawn != 0 ? asset.ink : std::uint8_t{ 0 }; }) | std::ranges::to<std::vector>() };
  return Image{ bits.Shape(), std::move(pixels), std::move(opaque) };
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
    return Region(file, asset, planes).and_then([&](Bytes const bytes) { return DecodePlanes(bytes, InterleavedLayout(asset.width_bytes, asset.height, planes)); });
  }
  case Layout::BITMAP:
    return Region(file, asset, 1).and_then([&](Bytes const bytes) { return DecodeBitmap(bytes, asset.width_bytes, asset.height); }).transform([&](Image const& bits) {
      return Narrowed(Inked(bits, asset), asset.width);
    });
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
    return DecodeMaskedSprite(MaskedSprite{ .image = *image, .mask = *mask, .width_bytes = asset.width_bytes, .height = asset.height });
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
  AddDialogs(builder);
  AddLogos(builder);
  AddMasks(builder);
  AddGlyphs(builder);
  AddProgramGlyphs(builder);
  _assets = builder.Take();
}

auto Catalogue::Find(std::string_view const id) const -> std::optional<Asset> {
  auto const found{ std::ranges::find(_assets, id, &Asset::id) };
  if (found == _assets.end()) return std::nullopt;
  return *found;
}

auto Catalogue::File(std::string_view const name) const -> Decoded<Bytes> {
  if (auto const table{ ProgramFile(name) }) return *table;
  return _source.get().File(name);
}

auto Catalogue::Decode(Asset const& asset) const -> Decoded<Image> {
  return File(asset.file).and_then([&](Bytes const file) { return DecodeAsset(file, asset); });
}

auto Catalogue::DecodeTexture(Asset const& asset) const -> Decoded<Texture> {
  auto const palette{ FindPalette(asset.palette) };
  if (!palette) return std::unexpected{ DecodeError::OUT_OF_RANGE };
  return Decode(asset).transform([&](Image const& image) { return ToTexture(image, *palette); });
}

}
