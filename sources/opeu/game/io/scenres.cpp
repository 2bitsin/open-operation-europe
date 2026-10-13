// Loads a scenario's text, the message pictures and the scenario's map, frame, tile and font resources. Files are
// named by their DS name strings; reads go to a DS offset or to a far segment the game allocated.
#include "io/scenres.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "io/resopen.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr std::uint32_t TEXT_RECORD{ 800 };            // a scenario text record in SCEDATA
constexpr Scenario      WINTER_MAP{ Scenario::BULGE }; // the map drawn with the winter font HEXFONTS

auto Open(Game& game, auto const& name) -> std::int16_t {
  return static_cast<std::int16_t>(static_cast<std::uint16_t>(opeu_open_resource(game, Near(Data(game), name))));
}

auto Seek(Game& game, std::int16_t handle, std::uint32_t position) -> void {
  opeu_dos_seek(static_cast<std::int16_t>(handle),
                static_cast<std::int32_t>((static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(position)))
                                           | static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(position >> 16))) << 16)),
                static_cast<std::int16_t>(SEEK_FROM_START));
}

auto ReadFar(Game& game, std::int16_t handle, std::uint16_t segment, std::uint16_t offset, std::uint16_t count) -> void {
  opeu_dos_read_far(static_cast<std::int16_t>(handle), segment, offset, count);
}
} // namespace

// Reads the 800-byte scenario text record number-1 (number 0 reads record 0) from the start of SCEDATA into the DS
// offset `destination`.
void opeu_load_scenario_text(Game& game, std::uint8_t number, std::uint16_t destination) {
  if (number) number--;
  auto const file{ Open(game, Data(game).scenario_filename) };
  Seek(game, file, number * TEXT_RECORD);
  opeu_dos_read(static_cast<std::int16_t>(file), game.State().Bytes(destination, TEXT_RECORD).data(), TEXT_RECORD);
  opeu_dos_close(static_cast<std::int16_t>(file));
}

// Loads MSGRP.DAT into the message picture segment.
void opeu_load_message_graphics(Game& game) {
  auto const& data{ Data(game) };
  auto const  file{ Open(game, data.msgrp_filename) };
  ReadFar(game, file, data.message_picture_segment, 0, 0x5b00);
  opeu_dos_close(static_cast<std::int16_t>(file));
}

// Loads the frame pictures, the scenario's brief and notes, its maps, hex display, unit icons and font.
void opeu_load_scenario_graphics(Game& game) {
  auto const&   data{ Data(game) };
  std::uint32_t map{ static_cast<std::uint32_t>(std::to_underlying(CurrentScenario(data))) };

  // MGAMEN: three shared frame blocks, then the map's own block (blocks of 512 bytes)
  auto file{ Open(game, data.mgamen_filename) };
  Seek(game, file, 0);
  ReadFar(game, file, data.frame_segment, 0, 0x400);
  Seek(game, file, 0x200);
  ReadFar(game, file, data.frame_segment, 0x400, 0xc00);
  Seek(game, file, 0xe00);
  ReadFar(game, file, data.frame_segment, 0x1000, 0xc00);
  Seek(game, file, static_cast<std::uint32_t>(data.mgamen_blocks[map]) << 9);
  ReadFar(game, file, data.frame_segment, 0x1c00, 0xc00);
  opeu_dos_close(static_cast<std::int16_t>(file));

  // the brief's 400 bytes run on into the notes, which the next read fills
  file = Open(game, data.scenario_filename);
  Seek(game, file, (map + 6) * TEXT_RECORD);
  opeu_dos_read(static_cast<std::int16_t>(file), game.State().Bytes(Near(data, data.scenario_brief), 400).data(), 400);
  Seek(game, file, map * TEXT_RECORD + 0x1450);
  opeu_dos_read(static_cast<std::int16_t>(file), game.State().Bytes(Near(data, data.scenario_notes), 400).data(), 400);
  opeu_dos_close(static_cast<std::int16_t>(file));

  file = Open(game, data.bigmap_filename);
  Seek(game, file, map * 0xc00);
  ReadFar(game, file, data.overview_tile_segment, 0, 0xc00);
  opeu_dos_close(static_cast<std::int16_t>(file));

  file = Open(game, data.midmap_filename);
  Seek(game, file, map * 0x6c00);
  ReadFar(game, file, data.tile_segment, 0, 0x6c00);
  opeu_dos_close(static_cast<std::int16_t>(file));

  file = Open(game, data.mappno_filename);
  Seek(game, file, map * 0x640);
  opeu_dos_read(static_cast<std::int16_t>(file), game.State().Bytes(Near(data, data.map_tiles), 0x640).data(), 0x640);
  opeu_dos_close(static_cast<std::int16_t>(file));

  file = Open(game, data.hexdisp_filename);
  Seek(game, file, map * 0x640);
  ReadFar(game, file, data.hex_display_segment, 0, 0x640);
  opeu_dos_close(static_cast<std::int16_t>(file));

  file = Open(game, data.hexunit_filename);
  ReadFar(game, file, data.icon_segment, 0, 0x8660);
  opeu_dos_close(static_cast<std::int16_t>(file));

  file = CurrentScenario(data) != WINTER_MAP ? Open(game, data.hexfont_filename) : Open(game, data.hexfonts_filename);
  ReadFar(game, file, data.sprite_segment, 0, 0xf136);
  opeu_dos_close(static_cast<std::int16_t>(file));
}
