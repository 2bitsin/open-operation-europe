// Ranged fire between battle elements: the firing sound, the tracers from the shooter to its target (or its target
// cell), and one shot: panel, tracers, damage, explosion, then losses, supplies and wear.
#include "map/btlrfire.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "io/sound.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcell.hpp"
#include "map/btlcmdr.hpp"
#include "map/btldraw.hpp"
#include "map/btlelem.hpp"
#include "map/btlline.hpp"
#include "map/btllist.hpp"
#include "map/btlrcalc.hpp"
#include "map/btlrpanl.hpp"
#include "map/btlspend.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <ranges>

using namespace opeu::game;

namespace {
// The battle dispatch's firing sounds.
constexpr uint8_t SOUND_HEAVY_GUN{ 0 };
constexpr uint8_t SOUND_LONG_GUN{ 1 };
constexpr uint8_t SOUND_FLAGGED{ 2 };
constexpr uint8_t SOUND_RIFLES{ 3 };
// ElementType::flags bit of the types with their own firing sound.
constexpr uint8_t TYPE_OWN_SOUND{ 0x02 };
// Tracers: the offset from a cell's corner to its centre, the line colour, the pauses.
constexpr uint16_t CENTRE_X{ 0x14 };
constexpr uint16_t CENTRE_Y{ 0x0d };
constexpr uint16_t TRACER_COLOR{ 0x17 };
constexpr uint8_t  MAX_TRACERS{ 5 };
constexpr uint8_t  FIRE_PAUSE{ 6 };
constexpr uint8_t  SILENT_PAUSE{ 2 };
// The explosion and the miss animations.
constexpr uint16_t HIT_ANIMATION{ 0x6fb8 };
constexpr uint16_t MISS_ANIMATION{ 0x79e0 };

// Tanks and artillery fire shells; the other categories fire rifles.
[[nodiscard]] auto FiresShells(ElementType const& type) -> bool {
  return IsOfCategory(type, Category::TANKS) || IsOfCategory(type, Category::ARTILLERY);
}

void Pause(Game& game, uint8_t units) {
  opeu_battle_pause(game, static_cast<uint8_t>(units));
}

// The screen point at the centre of a field cell.
[[nodiscard]] auto CellCentre(Game& game, GridCell cell) -> ScreenPoint {
  return { static_cast<uint16_t>(opeu_battle_cell_column(game, cell.x, cell.y) + CENTRE_X), static_cast<uint16_t>(opeu_battle_cell_row(game, cell.x, cell.y) + CENTRE_Y) };
}

// A line between two screen points.
struct ScreenLine {
  ScreenPoint from;
  ScreenPoint to;
};

// A shot of one element at another (0: at the shooter's goal).
struct Shot {
  uint16_t shooter;
  uint16_t target;
};

// A tracer XORed onto the screen, held a moment and XORed off again.
void FlashTracer(Game& game, ScreenLine const& line) {
  game.Machine().Display().Line(
      {
          { line.from.x, line.from.y },
          { line.to.x,   line.to.y   }
  },
      TRACER_COLOR);
  Pause(game, 1);
  game.Machine().Display().Line(
      {
          { line.from.x, line.from.y },
          { line.to.x,   line.to.y   }
  },
      TRACER_COLOR);
}

// A few tracers along the line, each jittered: one per three shots for infantry, per fifteen otherwise, at most five.
void Tracers(Game& game, Shot shot, ScreenLine const& line) {
  auto const& type{ ElementTypeOf(Data(game), shot.shooter) };
  auto const  shots{ opeu_battle_element_shots(game, shot.shooter, shot.target) };
  auto const  tracers{ std::min(static_cast<uint8_t>((IsOfCategory(type, Category::INFANTRY) ? shots / 3 : shots / 15) + 1), MAX_TRACERS) };
  for ([[maybe_unused]] auto const tracer : std::views::iota(uint8_t{ 0 }, tracers)) {
    auto const dx{ static_cast<uint16_t>(game.State().Generator().Bounded(8) - 4) };
    auto const dy{ static_cast<uint16_t>(game.State().Generator().Bounded(10) - 5) };
    auto const jitter{ [&](ScreenPoint point) { return ScreenPoint{ static_cast<uint16_t>(point.x + dx), static_cast<uint16_t>(point.y + dy) }; } };
    FlashTracer(game, { jitter(line.from), jitter(line.to) });
  }
  Pause(game, static_cast<uint8_t>(FIRE_PAUSE - tracers));
}

// The cell the shot goes to: the target's, or the shooter's goal.
[[nodiscard]] auto TargetCell(DataSegment& data, uint16_t shooter, uint16_t target) -> GridCell {
  if (target) return { PositionOf(data, target).x, PositionOf(data, target).y };
  return { PositionOf(data, shooter).goal_x, PositionOf(data, shooter).goal_y };
}
} // namespace

void opeu_battle_ranged_sound(Game& game, uint16_t shooter) {
  auto const& data{ Data(game) };
  if (!data.sound_enabled) {
    Pause(game, SILENT_PAUSE);
    return;
  }
  auto const& type{ ElementTypeOf(data, shooter) };
  if (type.flags & TYPE_OWN_SOUND) {
    opeu_battle_dispatch(game, SOUND_FLAGGED);
    return;
  }
  if (FiresShells(type))
    opeu_battle_dispatch(game, type.gun_range > 1 ? SOUND_LONG_GUN : SOUND_HEAVY_GUN);
  else if (type.category <= Bits(Category::SUPPLY))
    opeu_battle_dispatch(game, SOUND_RIFLES);
}

// Shown when the shooter's side is a player's or the shooter is spotted: one line for a long-range gun, otherwise a
// few jittered flashes.
void opeu_battle_ranged_tracers(Game& game, uint16_t shooter, uint16_t target) {
  auto& data{ Data(game) };
  opeu_battle_ranged_sound(game, shooter);
  if (data.side_status[opeu_battle_element_side(game, shooter)] != SIDE_PLAYED && !opeu_battle_element_flag2(game, shooter)) {
    Pause(game, FIRE_PAUSE);
  } else {
    auto const&      from{ PositionOf(data, shooter) };
    ScreenLine const line{ CellCentre(game, { from.x, from.y }), CellCentre(game, TargetCell(data, shooter, target)) };
    if (ElementTypeOf(data, shooter).gun_range > 1)
      opeu_battle_draw_line(game, line.from, line.to);
    else
      Tracers(game, { shooter, target }, line);
  }
  opeu_music_command7(game);
}

// frame: the routine's frame; the callees run below it.
void opeu_battle_ranged_fire(Game& game, uint16_t shooter, uint16_t target) {
  auto&      data{ Data(game) };
  auto const cell{ TargetCell(data, shooter, target) };
  opeu_battle_ranged_panel(game, shooter, target);
  opeu_battle_ranged_tracers(game, shooter, target);
  auto const  shot{ opeu_battle_ranged_damage(game, shooter, target) };
  auto const  losses{ shot.losses };
  auto const  animation{ losses ? HIT_ANIMATION : MISS_ANIMATION };
  auto const& type{ ElementTypeOf(data, shooter) };
  if (FiresShells(type)) {
    auto const heavy{ target && (IsOfCategory(ElementTypeOf(data, target), Category::INFANTRY) ? losses >= 30 : losses >= 10) };
    opeu_battle_animate_element(game, cell, { animation, target, true, heavy });
  } else if (type.category <= Bits(Category::SUPPLY)) {
    opeu_battle_animate_element(game, cell, { animation, target, false, false });
  }
  opeu_battle_panel_begin(game);
  data.battle_element_destroyed = 0;
  if (!losses) {
    opeu_battle_refresh_cell(game, cell.x, cell.y);
  } else if (!shot.into_bunker) {
    auto&      hit{ Record<Element>(data, target) };
    auto const before{ hit.quantity };
    uint16_t   quantity{ before };
    quantity     = SubWord(quantity, losses);
    hit.quantity = quantity;
    opeu_battle_spend_fuel(game, target, before);
    opeu_battle_spend_ammo(game, target, before);
    opeu_battle_spend_stores(game, target, before);
    if (!hit.quantity) {
      hit.damage = 0;
      opeu_battle_draw_cell(game, cell.x, cell.y);
      opeu_battle_wiped_notice(game, target);
      data.battle_element_destroyed = 1;
      auto& own{ Record<Element>(data, shooter) };
      own.training = AddByte(own.training, 1, RATING_LIMIT);
      own.morale   = AddByte(own.morale, 3, RATING_LIMIT);
    } else {
      // Vehicles take damage by the crew's skill.
      if (auto const category{ ElementTypeOf(data, target).category }) {
        auto const roll{ game.State().Generator().Bounded(300) };
        auto const skill{ opeu_battle_type_skill(game, target) };
        uint16_t   damage{ hit.damage };
        damage     = AddWord(damage, static_cast<uint16_t>(static_cast<uint16_t>(skill * losses) / static_cast<uint16_t>(700 - roll)), data.request_capacities[category]);
        hit.damage = damage;
      }
      opeu_battle_redraw_cell(game, cell, { target, opeu_battle_mode(game, target) });
    }
  } else {
    // A shot into a bunker: the garrison share there and the objective's strength take the losses.
    auto& objective{ Record<Objective>(data, opeu_find_objective_at(game, data.battle_x, data.battle_y)) };
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_EXTRA_COUNT)) {
      auto& extra{ BattleExtraAt(data, i) };
      if (extra.x == BATTLE_UNPLACED) break;
      if (extra.x == cell.x && extra.y == cell.y) {
        extra.count        = SubByte(extra.count, losses);
        objective.strength = SubByte(objective.strength, losses);
        opeu_battle_refresh_cell(game, cell.x, cell.y);
        break;
      }
    }
  }
  opeu_battle_set_order(game, shooter, 0);
}
