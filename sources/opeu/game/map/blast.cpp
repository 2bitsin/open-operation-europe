// The explosion at a map location, the blast erase and the blast pause, and the map segment's shared services.
//
// The blast centres the map on the location for the side's status view, plays sound cue 5 and animates: in the
// overview two 1 x 8 frames, in the tactical view three 3 x 24 sprites, each followed by a short wait. The erase is
// the far wrapper of the cell erase the damage reports use; the pause is the wait passed as a far callback to the
// daily notice.
#include "map/blast.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"

#include "core/dialogs.hpp"
#include "core/emsmap.hpp"
#include "core/memory.hpp"
#include "core/sideutil.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
enum class BlastService : uint16_t {
  SIDE_FOR_STATUS  = 0, // side -> the side whose view shows
  ARRIVAL_LOCATION = 1, // x, y, side
  SOUND            = 2, //
  WAIT             = 3, // units
  BLIT_FAR         = 4, // as MapService::BLIT_FAR
  SPRITE_COMPOSE   = 5, // as MapService::SPRITE_COMPOSE
  EFFECT_FINISH    = 6, //
  MAP_ERASE        = 7, // x, y
};

// The animation in the sprite segment: overview frames, tactical images and their masks.
constexpr uint16_t OVERVIEW_FRAMES{ 0x0260 };
constexpr uint16_t OVERVIEW_FRAME_BYTES{ 24 };
constexpr uint8_t  OVERVIEW_FRAME_COUNT{ 2 };
constexpr uint16_t VIEW_IMAGES{ 0x1fd0 };
constexpr uint16_t VIEW_IMAGE_BYTES{ 72 };
constexpr uint16_t VIEW_MASKS{ 0x20a8 };
constexpr uint16_t VIEW_MASK_BYTES{ 216 };
constexpr uint8_t  VIEW_FRAME_COUNT{ 3 };
constexpr uint16_t FRAME_WAIT{ 2 };
constexpr uint16_t PAUSE_WAIT{ 30 };
constexpr uint8_t  NONE{ 255 };

void SetStream(DataSegment& data, uint16_t offset) {
  data.stream_ptr[0] = offset;
  data.stream_ptr[1] = SPRITE_SEGMENT;
}
} // namespace

void opeu_location_blast(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint8_t side) {
  auto&      data{ Data(ds) };
  auto const shown{ static_cast<uint8_t>(Call(ds, event, BlastService::SIDE_FOR_STATUS, side)) };
  Call(ds, event, BlastService::ARRIVAL_LOCATION, x, y, shown);
  Call(ds, event, BlastService::SOUND);
  Call(ds, event, BlastService::WAIT, FRAME_WAIT);
  if (data.overview_side != NONE) {
    auto const pixel{ OverviewPixel({ x, y }) };
    for (uint8_t i{ 0 }; i < OVERVIEW_FRAME_COUNT; ++i) {
      SetStream(data, static_cast<uint16_t>(OVERVIEW_FRAMES + OVERVIEW_FRAME_BYTES * i));
      Call(ds, event, BlastService::BLIT_FAR, pixel.x, pixel.y, 1, 8, 4, data.stream_ptr[1], data.stream_ptr[0]);
      Call(ds, event, BlastService::WAIT, FRAME_WAIT);
    }
  } else {
    auto const pixel{ ViewPixel(data, { x, y }) };
    for (uint8_t i{ 0 }; i < VIEW_FRAME_COUNT; ++i) {
      SetStream(data, static_cast<uint16_t>(VIEW_IMAGES + VIEW_IMAGE_BYTES * i));
      Call(ds, event, BlastService::SPRITE_COMPOSE, pixel.x, pixel.y, 3, 24, data.stream_ptr[0], data.stream_ptr[1], static_cast<uint16_t>(VIEW_MASKS + VIEW_MASK_BYTES * i), SPRITE_SEGMENT, 0,
           data.bitmap_buffer, 0);
      Call(ds, event, BlastService::WAIT, FRAME_WAIT);
    }
  }
  Call(ds, event, BlastService::EFFECT_FINISH);
}

void opeu_blast_erase(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y) {
  Call(ds, event, BlastService::MAP_ERASE, x, y);
}

void opeu_blast_pause(uint8_t* ds, HostEvent event) {
  Call(ds, event, BlastService::WAIT, PAUSE_WAIT);
}

uint16_t svc_blast(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  switch (static_cast<BlastService>(id)) {
  case BlastService::SIDE_FOR_STATUS:  return opeu_side_for_status(ds, byte(0));
  case BlastService::ARRIVAL_LOCATION: opeu_arrival_location(ds, svc_mappos, byte(0), byte(1), byte(2)); return 0;
  case BlastService::SOUND:            opeu_sound_cue5(ds, svc_soundcue); return 0;
  case BlastService::WAIT:             opeu_delay_units(ds, svc_waitkey, byte(0)); return 0;
  case BlastService::BLIT_FAR:         return svc_map_plan(ds, "blast", std::to_underlying(MapService::BLIT_FAR), args, count);
  case BlastService::SPRITE_COMPOSE:   return svc_map_plan(ds, "blast", std::to_underlying(MapService::SPRITE_COMPOSE), args, count);
  case BlastService::EFFECT_FINISH:    opeu_effect_finish(ds, svc_sound); return 0;
  case BlastService::MAP_ERASE:        opeu_map_erase(ds, svc_maperase, byte(0), byte(1)); return 0;
  default:                             port_unhandled("blast", id, count);
  }
  return 0;
}

// The map segment's shared services (MapService). The VGA "far" routines take the segment before the offset; the
// compositor takes (offset, segment) pairs for the image, the mask and the target buffer.
uint16_t svc_map_plan(uint8_t* ds, const char* module, uint16_t kind, const uint16_t* args, uint16_t count) {
  auto const w{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const far{ [&](unsigned segment, unsigned offset) { return port_far(w(segment), w(offset)); } };
  auto const near{ [&](unsigned i) { return PORT_DS(w(i)); } };
  switch (static_cast<MapService>(kind)) {
  case MapService::DRAW_FAR:         opeu_draw_far(w(0), w(1), w(2), w(3), w(4), far(5, 6)); break;
  case MapService::DRAW_NEAR:        opeu_draw_near(w(0), w(1), w(2), w(3), w(4), near(5)); break;
  case MapService::RESTORE_NEAR:     koei_266c(w(0), w(1), w(2), w(3), w(4), near(5)); break;
  case MapService::SAVE_NEAR:        koei_2e72(w(0), w(1), w(2), w(3), near(4)); break;
  case MapService::GLYPH_NEAR:       opeu_vga_glyph(w(0), w(1), w(2), w(3), w(4), near(5)); break;
  case MapService::LINE:             opeu_ui_line(w(0), w(1), w(2), w(3), w(4)); break;
  case MapService::RESTORE_FAR:      opeu_bitmap_restore(w(0), w(1), w(2), w(3), w(4), far(5, 6)); break;
  case MapService::DRAW_MASKED_NEAR: opeu_code_0000_28c5(w(0), w(1), w(2), w(3), w(4), near(5)); break;
  case MapService::SPRITE_COMPOSE:   opeu_sprite_compose(w(0), w(1), w(2), w(3), far(5, 4), far(7, 6), far(9, 8), w(10)); break;
  case MapService::BLIT_FAR:         opeu_blit_far(w(0), w(1), w(2), w(3), w(4), far(5, 6)); break;
  case MapService::PATTERN_NEAR:     koei_27c2(w(0), w(1), w(2), w(3), w(4), near(5)); break;
  case MapService::SCREEN_SHIFT:     koei_3bc5(w(0), w(1), w(2), w(3)); break;
  case MapService::WAIT:             opeu_delay_units(ds, svc_waitkey, static_cast<uint8_t>(w(0))); break;
  case MapService::CONTROL_MESSAGE:  opeu_control_message(ds, static_cast<DialogHost const*>(port_dialogs_host()), w(0), w(1)); break;
  case MapService::STATUS_MODE:      opeu_turn_frame(ds, static_cast<DialogHost const*>(port_dialogs_host()), static_cast<uint8_t>(w(0))); break;
  case MapService::VIEW_PREPARE:     opeu_view_prepare(ds, svc_emsmap); break;
  case MapService::READ_FAR_BYTES:   opeu_read_far_bytes(ds, port_far(w(1), 0), w(0), w(2), w(3)); break;
  default:                           port_unhandled(module, kind, count);
  }
  return 0;
}

auto port_routine_opeu_blast_pause(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_blast_pause(ds, svc_blast);
  return 0;
}
