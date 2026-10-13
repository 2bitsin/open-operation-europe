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

enum Service : std::uint16_t {
  OPEN     = 0, // (name) -> handle
  SEEK     = 1, // (handle, position low, position high, origin)
  READ     = 2, // (handle, DS offset, count)
  READ_FAR = 3, // (handle, segment, offset, count)
  CLOSE    = 4, // (handle)
};

constexpr std::uint32_t TEXT_RECORD{ 800 };            // a scenario text record in SCEDATA
constexpr Scenario      WINTER_MAP{ Scenario::BULGE }; // the map drawn with the winter font HEXFONTS

auto Open(Game& game, HostEvent event, auto const& name) -> std::int16_t {
  return static_cast<std::int16_t>(Call(game, event, OPEN, Near(Data(game), name)));
}

auto Seek(Game& game, HostEvent event, std::int16_t handle, std::uint32_t position) -> void {
  Call(game, event, SEEK, handle, static_cast<std::uint16_t>(position), static_cast<std::uint16_t>(position >> 16), SEEK_FROM_START);
}

auto ReadFar(Game& game, HostEvent event, std::int16_t handle, std::uint16_t segment, std::uint16_t offset, std::uint16_t count) -> void {
  Call(game, event, READ_FAR, handle, segment, offset, count);
}
} // namespace

// Reads the 800-byte scenario text record number-1 (number 0 reads record 0) from the start of SCEDATA into the DS
// offset `destination`.
void opeu_load_scenario_text(Game& game, HostEvent event, std::uint8_t number, std::uint16_t destination) {
  if (number) number--;
  auto const file{ Open(game, event, Data(game).scenario_filename) };
  Seek(game, event, file, number * TEXT_RECORD);
  Call(game, event, READ, file, destination, TEXT_RECORD);
  Call(game, event, CLOSE, file);
}

// Loads MSGRP.DAT into the message picture segment.
void opeu_load_message_graphics(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  auto const  file{ Open(game, event, data.msgrp_filename) };
  ReadFar(game, event, file, data.message_picture_segment, 0, 0x5b00);
  Call(game, event, CLOSE, file);
}

// Loads the frame pictures, the scenario's brief and notes, its maps, hex display, unit icons and font.
void opeu_load_scenario_graphics(Game& game, HostEvent event) {
  auto const&   data{ Data(game) };
  std::uint32_t map{ static_cast<std::uint32_t>(std::to_underlying(CurrentScenario(data))) };

  // MGAMEN: three shared frame blocks, then the map's own block (blocks of 512 bytes)
  auto file{ Open(game, event, data.mgamen_filename) };
  Seek(game, event, file, 0);
  ReadFar(game, event, file, data.frame_segment, 0, 0x400);
  Seek(game, event, file, 0x200);
  ReadFar(game, event, file, data.frame_segment, 0x400, 0xc00);
  Seek(game, event, file, 0xe00);
  ReadFar(game, event, file, data.frame_segment, 0x1000, 0xc00);
  Seek(game, event, file, static_cast<std::uint32_t>(data.mgamen_blocks[map]) << 9);
  ReadFar(game, event, file, data.frame_segment, 0x1c00, 0xc00);
  Call(game, event, CLOSE, file);

  // the brief's 400 bytes run on into the notes, which the next read fills
  file = Open(game, event, data.scenario_filename);
  Seek(game, event, file, (map + 6) * TEXT_RECORD);
  Call(game, event, READ, file, Near(data, data.scenario_brief), 400);
  Seek(game, event, file, map * TEXT_RECORD + 0x1450);
  Call(game, event, READ, file, Near(data, data.scenario_notes), 400);
  Call(game, event, CLOSE, file);

  file = Open(game, event, data.bigmap_filename);
  Seek(game, event, file, map * 0xc00);
  ReadFar(game, event, file, data.overview_tile_segment, 0, 0xc00);
  Call(game, event, CLOSE, file);

  file = Open(game, event, data.midmap_filename);
  Seek(game, event, file, map * 0x6c00);
  ReadFar(game, event, file, data.tile_segment, 0, 0x6c00);
  Call(game, event, CLOSE, file);

  file = Open(game, event, data.mappno_filename);
  Seek(game, event, file, map * 0x640);
  Call(game, event, READ, file, Near(data, data.map_tiles), 0x640);
  Call(game, event, CLOSE, file);

  file = Open(game, event, data.hexdisp_filename);
  Seek(game, event, file, map * 0x640);
  ReadFar(game, event, file, data.hex_display_segment, 0, 0x640);
  Call(game, event, CLOSE, file);

  file = Open(game, event, data.hexunit_filename);
  ReadFar(game, event, file, data.icon_segment, 0, 0x8660);
  Call(game, event, CLOSE, file);

  file = CurrentScenario(data) != WINTER_MAP ? Open(game, event, data.hexfont_filename) : Open(game, event, data.hexfonts_filename);
  ReadFar(game, event, file, data.sprite_segment, 0, 0xf136);
  Call(game, event, CLOSE, file);
}

auto svc_scenres(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (id) {
  case OPEN: return static_cast<std::uint16_t>(opeu_open_resource(game, svc_resopen, Arg(args, count, 0)));
  case SEEK:
    return static_cast<std::uint16_t>(opeu_dos_seek(static_cast<std::int16_t>(Arg(args, count, 0)), static_cast<std::int32_t>(Arg32(args, count, 1)), static_cast<std::int16_t>(Arg(args, count, 3))));
  case READ: return static_cast<std::uint16_t>(opeu_dos_read(static_cast<std::int16_t>(Arg(args, count, 0)), game.State().Bytes(Arg(args, count, 1), Arg(args, count, 2)).data(), Arg(args, count, 2)));
  case READ_FAR: return static_cast<std::uint16_t>(opeu_dos_read_far(static_cast<std::int16_t>(Arg(args, count, 0)), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)));
  case CLOSE:    return static_cast<std::uint16_t>(opeu_dos_close(static_cast<std::int16_t>(Arg(args, count, 0))));
  default:       port_unhandled("scenres", id, count);
  }
  return 0;
}
