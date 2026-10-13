// The battle map's set-up, run once per battle: the per-battle state cleared, the 14 x 14 terrain read from the hex
// map and translated, the objective's garrison spread over its cells, then the map, its border and the title drawn.
#include "map/btlmap.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "types.hpp"

#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/resopen.hpp"
#include "map/btlcell.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
enum class MapSetupService : uint16_t {
  BACKDROP          = 0,  // colour
  SPRITE            = 1,  // x, y, w, h, image offset, segment, mask offset, segment, target offset, segment, mode
  OPEN_RESOURCE     = 2,  // DS offset of the name -> handle
  SEEK              = 3,  // handle, position (two words), origin
  READ              = 4,  // handle, DS destination, size
  CLOSE             = 5,  // handle
  TEXT_POSITION     = 6,  // x, y
  CONTEXT_FRAME     = 7,  // x, y, width
  TEXT              = 8,  // DS offset
  PRINT             = 9,  // format, words...
  FIND_OBJECTIVE_AT = 10, // x, y -> objective
  VGA_CLEAR         = 11, //
  PALETTE_TITLE     = 12, //
  DRAW_CELL_ABOVE   = 13, // x, y
  DRAW_CELL         = 14, // x, y
  FAR_BYTE          = 15, // segment, offset -> the byte there
};

constexpr uint8_t  BORDER_COLOR{ 1 };
constexpr uint16_t BORDER_SPRITE_WIDTH{ 24 };
constexpr uint16_t BORDER_SPRITE_HEIGHT{ 20 };
constexpr uint16_t LEFT_BORDER_IMAGE{ 0x84f8 };
constexpr uint16_t LEFT_BORDER_MASK{ 0x8408 };
constexpr uint16_t RIGHT_BORDER_IMAGE{ 0x8624 };
constexpr uint16_t RIGHT_BORDER_MASK{ 0x8534 };
constexpr uint8_t  LAST_HEX_CODE{ 0x2a };
// The hex map's flag bits, kept through the translation.
constexpr uint8_t  HEX_FLAG_MASK{ 0xc0 };
constexpr uint32_t TERRAIN_RECORD{ BATTLE_WIDTH * BATTLE_WIDTH };
// Hex codes the battle keeps only on a city or bridge cell; elsewhere they become the replacement.
constexpr uint8_t TOWN_CODE_A{ 0x1f };
constexpr uint8_t TOWN_CODE_A_ELSEWHERE{ 0x15 };
constexpr uint8_t TOWN_CODE_B{ 0x20 };
constexpr uint8_t TOWN_CODE_B_ELSEWHERE{ 0x18 };
// The weather of a hurricane in the summer months of the Normandy scenario.
constexpr uint8_t  STORM{ 5 };
constexpr uint16_t TITLE_X{ 0x32 };
constexpr uint16_t TITLE_FRAME_WIDTH{ 0x1b };
constexpr uint16_t PLACE_FRAME_Y{ 0x20 };
constexpr uint16_t YEAR_BASE{ 40 };
constexpr uint8_t  NO_DIRECTION{ 0xff };

[[nodiscard]] auto StrategicTerrain(DataSegment const& data) -> uint8_t {
  return MapCellAt(data, data.battle_x, data.battle_y) & Bits(MapCell::TERRAIN_MASK);
}
} // namespace

// The two diagonal rows of border sprites.
void opeu_battle_draw_border(uint8_t* ds, HostEvent event) {
  auto&      data{ Data(ds) };
  auto const sprite{ [&](uint16_t x, uint16_t y, uint16_t image, uint16_t mask) {
    Call(ds, event, MapSetupService::SPRITE, x, y, BORDER_SPRITE_WIDTH, BORDER_SPRITE_HEIGHT, image, data.icon_segment, mask, data.icon_segment, 0, data.bitmap_buffer, 1);
  } };
  Call(ds, event, MapSetupService::BACKDROP, BORDER_COLOR);
  for (uint16_t a{ 0 }; a < BATTLE_WIDTH; ++a) sprite(static_cast<uint16_t>((a + 2) * 20), static_cast<uint16_t>((a + 16) * 13), LEFT_BORDER_IMAGE, LEFT_BORDER_MASK);
  for (uint16_t b{ 0 }; b < BATTLE_WIDTH; ++b) sprite(static_cast<uint16_t>((29 - b) * 20), static_cast<uint16_t>((b + 16) * 13), RIGHT_BORDER_IMAGE, RIGHT_BORDER_MASK);
}

// A hex map terrain code as the battle's (0 beyond the table).
uint8_t opeu_hex_terrain_code(uint8_t* ds, HostEvent, uint8_t code) {
  if (code > LAST_HEX_CODE) code = 0;
  return Data(ds).hex_terrain_codes[code];
}

// The battle's record of HEXMAP.DAT is the strategic cell's hex display byte - 1 past the scenario's first record.
void opeu_battle_load_terrain(uint8_t* ds, HostEvent event) {
  auto& data{ Data(ds) };
  data.stream_ptr[1] = data.hex_display_segment;
  data.stream_ptr[0] = static_cast<uint16_t>(data.battle_y * MAP_WIDTH + data.battle_x);
  data.hexmap_index  = static_cast<uint8_t>(Call(ds, event, MapSetupService::FAR_BYTE, data.stream_ptr[1], data.stream_ptr[0]));
  auto const position{ (static_cast<int32_t>(data.hexmap_index - 1) + data.hexmap_offsets[std::to_underlying(CurrentScenario(data))]) * static_cast<int32_t>(TERRAIN_RECORD) };
  auto const handle{ static_cast<int16_t>(Call(ds, event, MapSetupService::OPEN_RESOURCE, Near(data, data.hexmap_filename))) };
  Call(ds, event, MapSetupService::SEEK, handle, static_cast<uint16_t>(position), static_cast<uint16_t>(static_cast<uint32_t>(position) >> 16), 0);
  Call(ds, event, MapSetupService::READ, handle, Near(data, data.battle_terrain), TERRAIN_RECORD);
  Call(ds, event, MapSetupService::CLOSE, handle);
  for (unsigned y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (unsigned x{ 0 }; x < BATTLE_WIDTH; ++x) {
      auto&      cell{ BattleTerrainAt(data, x, y) };
      auto const flags{ static_cast<uint8_t>(cell & HEX_FLAG_MASK) };
      auto const code{ opeu_hex_terrain_code(ds, event, static_cast<uint8_t>(cell & BATTLE_TERRAIN_MASK)) };
      cell = static_cast<uint8_t>(code | flags);
    }
  for (unsigned y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (unsigned x{ 0 }; x < BATTLE_WIDTH; ++x) {
      auto&      cell{ BattleTerrainAt(data, x, y) };
      auto const code{ static_cast<uint8_t>(cell & BATTLE_TERRAIN_MASK) };
      if (code != TOWN_CODE_A && code != TOWN_CODE_B) continue;
      if (StrategicTerrain(data) == std::to_underlying(Terrain::BRIDGE) || StrategicTerrain(data) == std::to_underlying(Terrain::CITY)) continue;
      cell &= HEX_FLAG_MASK;
      cell |= code == TOWN_CODE_A ? TOWN_CODE_A_ELSEWHERE : TOWN_CODE_B_ELSEWHERE;
    }
}

// The border, the top row of half cells, then every row.
void opeu_battle_draw_map(uint8_t* ds, HostEvent event) {
  opeu_battle_draw_border(ds, event);
  auto const above{ [&](unsigned x, unsigned y) { Call(ds, event, MapSetupService::DRAW_CELL_ABOVE, static_cast<uint8_t>(x), static_cast<uint8_t>(y)); } };
  above(0, 0);
  for (unsigned x{ 0 }; x < BATTLE_WIDTH; ++x) above(x + 1, 0);
  for (unsigned y{ 0 }; y < BATTLE_WIDTH; ++y) {
    above(0, y + 1);
    for (unsigned x{ 0 }; x < BATTLE_WIDTH; ++x) Call(ds, event, MapSetupService::DRAW_CELL, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
  }
}

// "<weather> m.d.y hh:00" and the objective's name or the terrain, with the map coordinates.
void opeu_battle_draw_title(uint8_t* ds, HostEvent event) {
  auto&      data{ Data(ds) };
  auto const weather{ data.weather == STORM && CurrentScenario(data) == Scenario::D_DAY && data.month >= 7 && data.month <= 9 ? Near(data, data.battle_hurricane_text)
                                                                                                                              : data.weather_names[data.weather] };
  Call(ds, event, MapSetupService::TEXT_POSITION, TITLE_X, 0);
  Call(ds, event, MapSetupService::CONTEXT_FRAME, 0, 0, TITLE_FRAME_WIDTH);
  Call(ds, event, MapSetupService::TEXT, Near(data, data.battle_title_style));
  Call(ds, event, MapSetupService::PRINT, Near(data, data.battle_title_format), data.month, data.day, data.year + YEAR_BASE, data.hour, weather);
  Call(ds, event, MapSetupService::CONTEXT_FRAME, 0, PLACE_FRAME_Y, TITLE_FRAME_WIDTH);
  auto const x{ static_cast<uint16_t>(data.map_scale * data.battle_x) };
  auto const y{ static_cast<uint16_t>(data.map_scale * data.battle_y) };
  if (auto const objective{ Call(ds, event, MapSetupService::FIND_OBJECTIVE_AT, data.battle_x, data.battle_y) })
    Call(ds, event, MapSetupService::PRINT, Near(data, data.battle_place_format), objective, x, y);
  else
    Call(ds, event, MapSetupService::PRINT, Near(data, data.battle_terrain_format), data.terrain_names[StrategicTerrain(data)], x, y);
  Call(ds, event, MapSetupService::TEXT, Near(data, data.battle_title_end));
}

// The objective's garrison shared among its city cells: each gets the share, the first ones one more for the
// remainder; the unused records are ff ff ff.
void opeu_battle_garrison_cells(uint8_t* ds, HostEvent event) {
  auto&      data{ Data(ds) };
  auto const clear{ [&](unsigned i) {
    auto& extra{ BattleExtraAt(data, i) };
    extra.x     = BATTLE_UNPLACED;
    extra.y     = BATTLE_UNPLACED;
    extra.count = BATTLE_UNPLACED;
  } };
  auto const site{ Call(ds, event, MapSetupService::FIND_OBJECTIVE_AT, data.battle_x, data.battle_y) };
  if (!site) {
    for (unsigned i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i) clear(i);
    return;
  }
  uint8_t cells{ 0 };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x)
      if (BattleTerrainAt(data, x, y) == BATTLE_CITY) {
        auto& extra{ BattleExtraAt(data, cells) };
        extra.x = x;
        extra.y = y;
        ++cells;
      }
  auto const garrison{ Record<Objective>(data, site).strength };
  uint8_t    share{ 0 };
  if (cells) share = static_cast<uint8_t>(garrison / cells);
  for (uint8_t i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i)
    if (i < cells)
      BattleExtraAt(data, i).count = share;
    else
      clear(i);
  for (uint8_t i{ 0 }; Record<Objective>(data, site).strength - share * cells > i; ++i) ++BattleExtraAt(data, i).count;
}

void opeu_battle_reset_state(uint8_t* ds, HostEvent) {
  auto& data{ Data(ds) };
  data.battle_direction    = NO_DIRECTION;
  data.battle_hq_intact[0] = 0;
  data.battle_hq_intact[1] = 0;
  for (unsigned y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (unsigned x{ 0 }; x < BATTLE_WIDTH; ++x) BattleCellAt(data, x, y) &= static_cast<uint8_t>(~Bits(BattleCell::OCCUPANT_MASK));
  for (unsigned side{ 0 }; side < SIDE_COUNT; ++side)
    for (unsigned i{ 0 }; i < BATTLE_LIST_LENGTH; ++i) {
      data.battle_ai_plans[side * BATTLE_LIST_LENGTH + i]   = 0;
      data.battle_ai_targets[side * BATTLE_LIST_LENGTH + i] = 0;
    }
  data.battle_selected          = 0;
  data.battle_lost_element      = 0;
  data.battle_lost_unit         = 0;
  data.battle_exit_direction[0] = NO_SIDE;
  data.battle_element_destroyed = 0;
  data.battle_side_routed[0]    = 0;
  data.battle_side_routed[1]    = 0;
}

// The entry from opeu_notice_one.
void opeu_battle_map_setup(uint8_t* ds, HostEvent event) {
  opeu_battle_reset_state(ds, event);
  opeu_battle_load_terrain(ds, event);
  opeu_battle_garrison_cells(ds, event);
  Call(ds, event, MapSetupService::VGA_CLEAR);
  Call(ds, event, MapSetupService::PALETTE_TITLE);
  opeu_battle_draw_map(ds, event);
  opeu_battle_draw_title(ds, event);
}

uint16_t svc_btlmap(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const w{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(w(i)); } };
  switch (static_cast<MapSetupService>(id)) {
  case MapSetupService::BACKDROP:          opeu_backdrop_fill_color(ds, static_cast<UiHost const*>(port_uiprim_host()), byte(0)); return 0;
  case MapSetupService::SPRITE:            koei_3f26(w(0), w(1), w(2), w(3), port_far(w(5), w(4)), port_far(w(7), w(6)), port_far(w(9), w(8)), w(10)); return 0;
  case MapSetupService::OPEN_RESOURCE:     return static_cast<uint16_t>(opeu_open_resource(ds, svc_resopen, w(0)));
  case MapSetupService::SEEK:              return static_cast<uint16_t>(opeu_dos_seek(static_cast<int16_t>(w(0)), static_cast<int32_t>(Arg32(args, count, 1)), static_cast<int16_t>(w(3))));
  case MapSetupService::READ:              return static_cast<uint16_t>(opeu_dos_read(static_cast<int16_t>(w(0)), PORT_DS(w(1)), w(2)));
  case MapSetupService::CLOSE:             return static_cast<uint16_t>(opeu_dos_close(static_cast<int16_t>(w(0))));
  case MapSetupService::TEXT_POSITION:     opeu_army_position(ds, static_cast<TextHost*>(port_textout_host()), static_cast<short>(w(0)), static_cast<short>(w(1))); return 0;
  case MapSetupService::CONTEXT_FRAME:     opeu_context_frame(ds, static_cast<UiHost const*>(port_uiprim_host()), w(0), w(1), w(2)); return 0;
  case MapSetupService::TEXT:              opeu_ui_text(ds, static_cast<TextHost*>(port_textout_host()), w(0)); return 0;
  case MapSetupService::PRINT:             opeu_cursor_print(ds, { args, count }); return 0;
  case MapSetupService::FIND_OBJECTIVE_AT: return opeu_find_objective_at(ds, byte(0), byte(1));
  case MapSetupService::VGA_CLEAR:         return static_cast<uint16_t>(opeu_vga_clear());
  case MapSetupService::PALETTE_TITLE:     opeu_palette_title_screen(ds, static_cast<UiHost const*>(port_uiprim_host())); return 0;
  case MapSetupService::DRAW_CELL_ABOVE:   opeu_battle_draw_cell_above(ds, svc_btlcell, byte(0), byte(1)); return 0;
  case MapSetupService::DRAW_CELL:         opeu_battle_draw_cell(ds, svc_btlcell, byte(0), byte(1)); return 0;
  case MapSetupService::FAR_BYTE:          return *port_far(w(0), w(1));
  default:                                 port_unhandled("btlmap", id, count);
  }
  return 0;
}

auto port_routine_opeu_battle_map_setup(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_map_setup(ds, svc_btlmap);
  return 0;
}
