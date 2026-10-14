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

#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/resopen.hpp"
#include "map/btlcell.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {

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
void opeu_battle_draw_border(Game& game) {
  auto&      data{ Data(game) };
  auto const sprite{ [&](uint16_t x, uint16_t y, uint16_t image, uint16_t mask) {
    game.Machine().Display().ComposeShiftedSprite({ static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), BORDER_SPRITE_WIDTH, BORDER_SPRITE_HEIGHT },
                                                  { .and_mask = FarBytes(game, static_cast<std::uint16_t>(data.icon_segment), static_cast<std::uint16_t>(image)),
                                                    .or_image = FarBytes(game, static_cast<std::uint16_t>(data.icon_segment), static_cast<std::uint16_t>(mask)),
                                                    .buffer   = FarBytes(game, static_cast<std::uint16_t>(data.bitmap_buffer), 0),
                                                    .flags    = 1 });
  } };
  opeu_backdrop_fill_color(game, static_cast<uint8_t>(BORDER_COLOR));
  for (auto const a : std::views::iota(uint16_t{ 0 }, BATTLE_WIDTH)) sprite(static_cast<uint16_t>((a + 2) * 20), static_cast<uint16_t>((a + 16) * 13), LEFT_BORDER_IMAGE, LEFT_BORDER_MASK);
  for (auto const b : std::views::iota(uint16_t{ 0 }, BATTLE_WIDTH)) sprite(static_cast<uint16_t>((29 - b) * 20), static_cast<uint16_t>((b + 16) * 13), RIGHT_BORDER_IMAGE, RIGHT_BORDER_MASK);
}

// A hex map terrain code as the battle's (0 beyond the table).
uint8_t opeu_hex_terrain_code(Game& game, uint8_t code) {
  if (code > LAST_HEX_CODE) code = 0;
  return Data(game).hex_terrain_codes[code];
}

// The battle's record of HEXMAP.DAT is the strategic cell's hex display byte - 1 past the scenario's first record.
void opeu_battle_load_terrain(Game& game) {
  auto& data{ Data(game) };
  data.stream_ptr[1] = data.hex_display_segment;
  data.stream_ptr[0] = static_cast<uint16_t>(data.battle_y * MAP_WIDTH + data.battle_x);
  data.hexmap_index  = FarBytes(game, static_cast<std::uint16_t>(data.stream_ptr[1]), static_cast<std::uint16_t>(data.stream_ptr[0])).front();
  auto const position{ (static_cast<int32_t>(data.hexmap_index - 1) + data.hexmap_offsets[std::to_underlying(CurrentScenario(data))]) * static_cast<int32_t>(TERRAIN_RECORD) };
  auto const handle{ opeu_open_resource(game, Near(data, data.hexmap_filename)) };
  auto&      files{ game.Machine().Files() };
  files.Seek(HandleOf(handle), position, services::SeekOrigin::START);
  files.Read(HandleOf(handle), game.State().Bytes(Near(data, data.battle_terrain), TERRAIN_RECORD));
  files.Close(HandleOf(handle));
  for (auto const y : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH)) {
      auto&      cell{ BattleTerrainAt(data, x, y) };
      auto const flags{ static_cast<uint8_t>(cell & HEX_FLAG_MASK) };
      auto const code{ opeu_hex_terrain_code(game, static_cast<uint8_t>(cell & BATTLE_TERRAIN_MASK)) };
      cell = static_cast<uint8_t>(code | flags);
    }
  for (auto const y : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH)) {
      auto&      cell{ BattleTerrainAt(data, x, y) };
      auto const code{ static_cast<uint8_t>(cell & BATTLE_TERRAIN_MASK) };
      if (code != TOWN_CODE_A && code != TOWN_CODE_B) continue;
      if (StrategicTerrain(data) == std::to_underlying(Terrain::BRIDGE) || StrategicTerrain(data) == std::to_underlying(Terrain::CITY)) continue;
      cell &= HEX_FLAG_MASK;
      cell |= code == TOWN_CODE_A ? TOWN_CODE_A_ELSEWHERE : TOWN_CODE_B_ELSEWHERE;
    }
}

// The border, the top row of half cells, then every row.
void opeu_battle_draw_map(Game& game) {
  opeu_battle_draw_border(game);
  auto const above{ [&](unsigned x, unsigned y) { opeu_battle_draw_cell_above(game, static_cast<uint8_t>(static_cast<uint8_t>(x)), static_cast<uint8_t>(static_cast<uint8_t>(y))); } };
  above(0, 0);
  for (auto const x : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH)) above(x + 1, 0);
  for (auto const y : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH)) {
    above(0, y + 1);
    for (auto const x : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH)) opeu_battle_draw_cell(game, static_cast<uint8_t>(static_cast<uint8_t>(x)), static_cast<uint8_t>(static_cast<uint8_t>(y)));
  }
}

// "<weather> m.d.y hh:00" and the objective's name or the terrain, with the map coordinates.
void opeu_battle_draw_title(Game& game) {
  auto&      data{ Data(game) };
  auto const weather{ data.weather == STORM && CurrentScenario(data) == Scenario::D_DAY && data.month >= 7 && data.month <= 9 ? Near(data, data.battle_hurricane_text)
                                                                                                                              : data.weather_names[data.weather] };
  opeu_army_position(game, static_cast<short>(TITLE_X), static_cast<short>(0));
  opeu_context_frame(game, 0, 0, TITLE_FRAME_WIDTH);
  opeu_ui_text(game, static_cast<std::uint16_t>(Near(data, data.battle_title_style)));
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(Near(data, data.battle_title_format)), static_cast<std::uint16_t>(data.month), static_cast<std::uint16_t>(data.day),
                                                        static_cast<std::uint16_t>(data.year + YEAR_BASE), static_cast<std::uint16_t>(data.hour), static_cast<std::uint16_t>(weather) });
  opeu_context_frame(game, 0, PLACE_FRAME_Y, TITLE_FRAME_WIDTH);
  auto const x{ static_cast<uint16_t>(data.map_scale * data.battle_x) };
  auto const y{ static_cast<uint16_t>(data.map_scale * data.battle_y) };
  if (auto const objective{ opeu_find_objective_at(game, static_cast<uint8_t>(static_cast<std::uint16_t>(data.battle_x)), static_cast<uint8_t>(static_cast<std::uint16_t>(data.battle_y))) })
    opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.battle_place_format)), static_cast<std::uint16_t>(objective), static_cast<std::uint16_t>(x),
                                                          static_cast<std::uint16_t>(y) });
  else
    opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.battle_terrain_format)), static_cast<std::uint16_t>(data.terrain_names[StrategicTerrain(data)]),
                                                          static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y) });
  opeu_ui_text(game, static_cast<std::uint16_t>(Near(data, data.battle_title_end)));
}

// The objective's garrison shared among its city cells: each gets the share, the first ones one more for the
// remainder; the unused records are ff ff ff.
void opeu_battle_garrison_cells(Game& game) {
  auto&      data{ Data(game) };
  auto const clear{ [&](unsigned i) {
    auto& extra{ BattleExtraAt(data, i) };
    extra.x     = BATTLE_UNPLACED;
    extra.y     = BATTLE_UNPLACED;
    extra.count = BATTLE_UNPLACED;
  } };
  auto const site{ opeu_find_objective_at(game, static_cast<uint8_t>(static_cast<std::uint16_t>(data.battle_x)), static_cast<uint8_t>(static_cast<std::uint16_t>(data.battle_y))) };
  if (!site) {
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_EXTRA_COUNT)) clear(i);
    return;
  }
  uint8_t cells{ 0 };
  for (auto const y : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
      if (BattleTerrainAt(data, x, y) == BATTLE_CITY) {
        auto& extra{ BattleExtraAt(data, cells) };
        extra.x = x;
        extra.y = y;
        ++cells;
      }
  auto const garrison{ Record<Objective>(data, site).strength };
  uint8_t    share{ 0 };
  if (cells) share = static_cast<uint8_t>(garrison / cells);
  for (auto const i : std::views::iota(uint8_t{ 0 }, BATTLE_EXTRA_COUNT))
    if (i < cells)
      BattleExtraAt(data, i).count = share;
    else
      clear(i);
  for (auto const i : std::views::iota(0, std::max(0, Record<Objective>(data, site).strength - share * cells))) ++BattleExtraAt(data, static_cast<uint8_t>(i)).count;
}

void opeu_battle_reset_state(Game& game) {
  auto& data{ Data(game) };
  data.battle_direction    = NO_DIRECTION;
  data.battle_hq_intact[0] = 0;
  data.battle_hq_intact[1] = 0;
  for (auto const y : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(unsigned{ 0 }, BATTLE_WIDTH)) BattleCellAt(data, x, y) &= static_cast<uint8_t>(~Bits(BattleCell::OCCUPANT_MASK));
  for (auto const side : std::views::iota(unsigned{ 0 }, SIDE_COUNT))
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_LIST_LENGTH)) {
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
void opeu_battle_map_setup(Game& game) {
  opeu_battle_reset_state(game);
  opeu_battle_load_terrain(game);
  opeu_battle_garrison_cells(game);
  game.Machine().Display().Clear();
  opeu_palette_title_screen(game);
  opeu_battle_draw_map(game);
  opeu_battle_draw_title(game);
}
