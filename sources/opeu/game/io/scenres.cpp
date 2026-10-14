// Loads a scenario's text, the message pictures and the scenario's map, frame, tile and font resources. Files are
// named by their DS name strings; reads go to a DS offset or to a far segment the game allocated.
#include "io/scenres.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "io/resopen.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <span>
#include <utility>

using namespace opeu::game;

namespace {

constexpr std::uint32_t TEXT_RECORD{ 800 };            // a scenario text record in SCEDATA
constexpr Scenario      WINTER_MAP{ Scenario::BULGE }; // the map drawn with the winter font HEXFONTS
constexpr std::uint16_t BRIEF_SIZE{ 400 };

// A block of a far segment the game allocated.
struct FarBlock {
  std::uint16_t segment;
  std::uint16_t offset;
  std::uint16_t size;
};

// Opens the file the DS name string names, has `read` read from its handle and closes it.
auto WithFile(Game& game, auto const& name, std::invocable<std::int16_t> auto read) -> void {
  auto const file{ opeu_open_resource(game, Near(Data(game), name)) };
  read(file);
  game.Machine().Files().Close(HandleOf(file));
}

auto Seek(Game& game, std::int16_t file, std::uint32_t position) -> void {
  game.Machine().Files().Seek(HandleOf(file), static_cast<std::int32_t>(position), services::SeekOrigin::START);
}

auto ReadFar(Game& game, std::int16_t file, FarBlock const& block) -> void {
  game.Machine().Files().ReadFar(HandleOf(file), {
                                                     .at = { block.segment, block.offset },
                                                       .size = block.size
  });
}

// Reads into data-segment bytes.
auto ReadNear(Game& game, std::int16_t file, std::span<std::uint8_t> destination) -> void {
  game.Machine().Files().Read(HandleOf(file), destination);
}

// A file's block of `size` bytes at `position`, read into the far block.
auto ReadFarAt(Game& game, std::int16_t file, std::uint32_t position, FarBlock const& block) -> void {
  Seek(game, file, position);
  ReadFar(game, file, block);
}

// MGAMEN: three shared frame blocks, then the map's own block (blocks of 512 bytes).
auto LoadFrames(Game& game, std::uint32_t map) -> void {
  auto const& data{ Data(game) };
  WithFile(game, data.mgamen_filename, [&](std::int16_t file) {
    ReadFarAt(game, file, 0, { data.frame_segment, 0, 0x400 });
    ReadFarAt(game, file, 0x200, { data.frame_segment, 0x400, 0xc00 });
    ReadFarAt(game, file, 0xe00, { data.frame_segment, 0x1000, 0xc00 });
    ReadFarAt(game, file, static_cast<std::uint32_t>(data.mgamen_blocks[map]) << 9, { data.frame_segment, 0x1c00, 0xc00 });
  });
}

// The brief's 400 bytes run on into the notes, which the next read fills.
auto LoadBrief(Game& game, std::uint32_t map) -> void {
  auto const& data{ Data(game) };
  WithFile(game, data.scenario_filename, [&](std::int16_t file) {
    Seek(game, file, (map + 6) * TEXT_RECORD);
    ReadNear(game, file, game.State().Bytes(Near(data, data.scenario_brief), BRIEF_SIZE));
    Seek(game, file, map * TEXT_RECORD + 0x1450);
    ReadNear(game, file, game.State().Bytes(Near(data, data.scenario_notes), BRIEF_SIZE));
  });
}

// The overview and the tiles of the map, its tile numbers and its hex display.
auto LoadMaps(Game& game, std::uint32_t map) -> void {
  auto const& data{ Data(game) };
  WithFile(game, data.bigmap_filename, [&](std::int16_t file) { ReadFarAt(game, file, map * 0xc00, { data.overview_tile_segment, 0, 0xc00 }); });
  WithFile(game, data.midmap_filename, [&](std::int16_t file) { ReadFarAt(game, file, map * 0x6c00, { data.tile_segment, 0, 0x6c00 }); });
  WithFile(game, data.mappno_filename, [&](std::int16_t file) {
    Seek(game, file, map * 0x640);
    ReadNear(game, file, game.State().Bytes(Near(data, data.map_tiles), 0x640));
  });
  WithFile(game, data.hexdisp_filename, [&](std::int16_t file) { ReadFarAt(game, file, map * 0x640, { data.hex_display_segment, 0, 0x640 }); });
}

// The unit icons and the map font (the winter one for the winter map).
auto LoadIconsAndFont(Game& game) -> void {
  auto const& data{ Data(game) };
  WithFile(game, data.hexunit_filename, [&](std::int16_t file) { ReadFar(game, file, { data.icon_segment, 0, 0x8660 }); });
  auto const read_font{ [&](std::int16_t file) { ReadFar(game, file, { data.sprite_segment, 0, 0xf136 }); } };
  if (CurrentScenario(data) != WINTER_MAP)
    WithFile(game, data.hexfont_filename, read_font);
  else
    WithFile(game, data.hexfonts_filename, read_font);
}
} // namespace

// Reads the 800-byte scenario text record number-1 (number 0 reads record 0) from the start of SCEDATA into
// `destination`; a short file leaves the rest of the record zero.
void opeu_load_scenario_text(Game& game, std::uint8_t number, TextTarget destination) {
  if (number) number--;
  WithFile(game, Data(game).scenario_filename, [&](std::int16_t file) {
    Seek(game, file, number * TEXT_RECORD);
    std::array<std::uint8_t, TEXT_RECORD> record{};
    ReadNear(game, file, record);
    std::ranges::transform(record, destination.Characters(Data(game)).begin(), [](std::uint8_t byte) { return static_cast<char>(byte); });
  });
}

// Loads MSGRP.DAT into the message picture segment.
void opeu_load_message_graphics(Game& game) {
  auto const& data{ Data(game) };
  WithFile(game, data.msgrp_filename, [&](std::int16_t file) { ReadFar(game, file, { data.message_picture_segment, 0, 0x5b00 }); });
}

// Loads the frame pictures, the scenario's brief and notes, its maps, hex display, unit icons and font.
void opeu_load_scenario_graphics(Game& game) {
  auto const map{ static_cast<std::uint32_t>(std::to_underlying(CurrentScenario(Data(game)))) };
  LoadFrames(game, map);
  LoadBrief(game, map);
  LoadMaps(game, map);
  LoadIconsAndFont(game);
}
