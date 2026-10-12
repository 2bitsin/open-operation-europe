// Loads a scenario's text, the message pictures and the scenario's map, frame, tile and font resources. Files are
// named by their DS name strings; reads go to a DS offset or to a far segment the game allocated.
#include "io/scenres.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/emsmap.hpp"
#include "glue.hpp"
#include "io/resopen.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum Service : std::uint16_t {
  OPEN            = 0, // (name) -> handle
  SEEK            = 1, // (handle, position low, position high, origin)
  READ            = 2, // (handle, DS offset, count)
  READ_FAR        = 3, // (handle, segment, offset, count)
  CLOSE           = 4, // (handle)
  EMS_MAP         = 5,
  DISPLAY_PREPARE = 6,
  VIEW_PREPARE    = 7,
  GRAPHICS_WAIT   = 8,
};

constexpr std::uint16_t FROM_START{ 0 };
constexpr std::uint32_t TEXT_RECORD{ 800 }; // a scenario text record in SCEDATA
constexpr std::uint8_t  MAP_MASK{ 0x7f };   // the scenario byte's map number
constexpr std::uint8_t  WINTER_MAP{ 4 };    // the map drawn with the winter font HEXFONTS

auto Open(std::uint8_t* ds, HostEvent event, auto const& name) -> std::int16_t {
  return static_cast<std::int16_t>(Call(ds, event, OPEN, Near(Data(ds), name)));
}

auto Seek(std::uint8_t* ds, HostEvent event, std::int16_t handle, std::uint32_t position) -> void {
  Call(ds, event, SEEK, handle, static_cast<std::uint16_t>(position), static_cast<std::uint16_t>(position >> 16),
       FROM_START);
}

auto ReadFar(std::uint8_t* ds, HostEvent event, std::int16_t handle, std::uint16_t segment, std::uint16_t offset,
             std::uint16_t count) -> void {
  Call(ds, event, READ_FAR, handle, segment, offset, count);
}
} // namespace

// Reads the 800-byte scenario text record number-1 (number 0 reads record 0) from the start of SCEDATA into the DS
// offset `destination`.
void opeu_load_scenario_text(std::uint8_t* ds, HostEvent event, std::uint8_t number, std::uint16_t destination) {
  if (number) number--;
  auto const file{ Open(ds, event, Data(ds).scenario_filename) };
  Seek(ds, event, file, number * TEXT_RECORD);
  Call(ds, event, READ, file, destination, TEXT_RECORD);
  Call(ds, event, CLOSE, file);
}

// Loads MSGRP.DAT into the message picture segment.
void opeu_load_message_graphics(std::uint8_t* ds, HostEvent event) {
  auto const& data{ Data(ds) };
  auto const  file{ Open(ds, event, data.msgrp_filename) };
  Call(ds, event, EMS_MAP);
  ReadFar(ds, event, file, data.message_picture_segment, 0, 0x5b00);
  Call(ds, event, CLOSE, file);
}

// Loads the frame pictures, the scenario's brief and notes, its maps, hex display, unit icons and font.
void opeu_load_scenario_graphics(std::uint8_t* ds, HostEvent event) {
  auto const&   data{ Data(ds) };
  std::uint32_t map{ static_cast<std::uint32_t>(data.scenario & MAP_MASK) };

  // MGAMEN: three shared frame blocks, then the map's own block (blocks of 512 bytes)
  auto file{ Open(ds, event, data.mgamen_filename) };
  Seek(ds, event, file, 0);
  Call(ds, event, DISPLAY_PREPARE);
  ReadFar(ds, event, file, data.frame_segment, 0, 0x400);
  Seek(ds, event, file, 0x200);
  Call(ds, event, DISPLAY_PREPARE);
  ReadFar(ds, event, file, data.frame_segment, 0x400, 0xc00);
  Seek(ds, event, file, 0xe00);
  Call(ds, event, DISPLAY_PREPARE);
  ReadFar(ds, event, file, data.frame_segment, 0x1000, 0xc00);
  Seek(ds, event, file, static_cast<std::uint32_t>(data.mgamen_blocks[map]) << 9);
  Call(ds, event, DISPLAY_PREPARE);
  ReadFar(ds, event, file, data.frame_segment, 0x1c00, 0xc00);
  Call(ds, event, CLOSE, file);

  // the brief's 400 bytes run on into the notes, which the next read fills
  file = Open(ds, event, data.scenario_filename);
  Seek(ds, event, file, (map + 6) * TEXT_RECORD);
  Call(ds, event, READ, file, Near(data, data.scenario_brief), 400);
  Seek(ds, event, file, map * TEXT_RECORD + 0x1450);
  Call(ds, event, READ, file, Near(data, data.scenario_notes), 400);
  Call(ds, event, CLOSE, file);

  file = Open(ds, event, data.bigmap_filename);
  Seek(ds, event, file, map * 0xc00);
  ReadFar(ds, event, file, data.overview_tile_segment, 0, 0xc00);
  Call(ds, event, CLOSE, file);

  file = Open(ds, event, data.midmap_filename);
  Seek(ds, event, file, map * 0x6c00);
  Call(ds, event, VIEW_PREPARE);
  ReadFar(ds, event, file, data.tile_segment, 0, 0x6c00);
  Call(ds, event, CLOSE, file);

  file = Open(ds, event, data.mappno_filename);
  Seek(ds, event, file, map * 0x640);
  Call(ds, event, READ, file, Near(data, data.map_tiles), 0x640);
  Call(ds, event, CLOSE, file);

  file = Open(ds, event, data.hexdisp_filename);
  Seek(ds, event, file, map * 0x640);
  ReadFar(ds, event, file, data.hex_display_segment, 0, 0x640);
  Call(ds, event, CLOSE, file);

  file = Open(ds, event, data.hexunit_filename);
  ReadFar(ds, event, file, data.icon_segment, 0, 0x8660);
  Call(ds, event, CLOSE, file);

  file = (data.scenario & MAP_MASK) != WINTER_MAP ? Open(ds, event, data.hexfont_filename)
                                                  : Open(ds, event, data.hexfonts_filename);
  Call(ds, event, GRAPHICS_WAIT);
  ReadFar(ds, event, file, data.sprite_segment, 0, 0xf136);
  Call(ds, event, CLOSE, file);
}

auto svc_scenres(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (id) {
  case OPEN: return static_cast<std::uint16_t>(opeu_open_resource(ds, svc_resopen, Arg(args, count, 0)));
  case SEEK:
    return static_cast<std::uint16_t>(opeu_dos_seek(static_cast<std::int16_t>(Arg(args, count, 0)),
                                                    static_cast<std::int32_t>(Arg32(args, count, 1)),
                                                    static_cast<std::int16_t>(Arg(args, count, 3))));
  case READ:
    return static_cast<std::uint16_t>(opeu_dos_read(static_cast<std::int16_t>(Arg(args, count, 0)),
                                                    PORT_DS(Arg(args, count, 1)), Arg(args, count, 2)));
  case READ_FAR:
    return static_cast<std::uint16_t>(opeu_dos_read_far(static_cast<std::int16_t>(Arg(args, count, 0)),
                                                        Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)));
  case CLOSE:   return static_cast<std::uint16_t>(opeu_dos_close(static_cast<std::int16_t>(Arg(args, count, 0))));
  case EMS_MAP: opeu_ems_map_dialog(ds, svc_emsmap); break;
  case DISPLAY_PREPARE: opeu_display_prepare(ds, svc_emsmap); break;
  case VIEW_PREPARE:    opeu_view_prepare(ds, svc_emsmap); break;
  case GRAPHICS_WAIT:   opeu_gfx_wait(ds, svc_emsmap); break;
  default:              port_unhandled("scenres", id, count);
  }
  return 0;
}
