// Ranged fire between battle elements: the firing sound, the tracers from the shooter to its target (or its target
// cell), and one shot: panel, tracers, damage, explosion, then losses, supplies and wear.
#include "map/btlrfire.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
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
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

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
  opeu_battle_pause(game, svc_btlcalc, static_cast<uint8_t>(units));
}

void Line(Game& game, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  opeu_ui_line(x1, y1, x2, y2, TRACER_COLOR);
}

// The cell the shot goes to: the target's, or the shooter's goal.
void TargetCell(DataSegment& data, uint16_t shooter, uint16_t target, uint8_t& x, uint8_t& y) {
  if (target) {
    auto const& position{ PositionOf(data, target) };
    x = position.x;
    y = position.y;
  } else {
    auto const& position{ PositionOf(data, shooter) };
    x = position.goal_x;
    y = position.goal_y;
  }
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
  if (data.side_status[opeu_battle_element_side(game, shooter)] == SIDE_PLAYED || opeu_battle_element_flag2(game, shooter)) {
    auto const& from{ PositionOf(data, shooter) };
    auto const  from_x{ static_cast<uint16_t>(opeu_battle_cell_column(game, from.x, from.y) + CENTRE_X) };
    auto const  from_y{ static_cast<uint16_t>(opeu_battle_cell_row(game, from.x, from.y) + CENTRE_Y) };
    uint8_t     x;
    uint8_t     y;
    TargetCell(data, shooter, target, x, y);
    auto const  to_x{ static_cast<uint16_t>(opeu_battle_cell_column(game, x, y) + CENTRE_X) };
    auto const  to_y{ static_cast<uint16_t>(opeu_battle_cell_row(game, x, y) + CENTRE_Y) };
    auto const& type{ ElementTypeOf(data, shooter) };
    if (type.gun_range > 1) {
      opeu_battle_draw_line(game, static_cast<int16_t>(from_x), static_cast<int16_t>(from_y), static_cast<int16_t>(to_x), static_cast<int16_t>(to_y));
    } else {
      auto tracers{ static_cast<uint8_t>(opeu_battle_element_shots(game, svc_btlcalc, shooter, target)) };
      tracers = static_cast<uint8_t>((IsOfCategory(type, Category::INFANTRY) ? tracers / 3 : tracers / 15) + 1);
      if (tracers > MAX_TRACERS) tracers = MAX_TRACERS;
      for (uint8_t i{ 0 }; i < tracers; ++i) {
        auto const dx{ static_cast<uint16_t>(opeu_rand_bounded(8) - 4) };
        auto const dy{ static_cast<uint16_t>(opeu_rand_bounded(10) - 5) };
        // Drawn twice: the line XORs onto the screen and off again.
        for (int pass{ 0 }; pass < 2; ++pass) {
          Line(game, from_x + dx, from_y + dy, to_x + dx, to_y + dy);
          if (!pass) Pause(game, 1);
        }
      }
      Pause(game, static_cast<uint8_t>(FIRE_PAUSE - tracers));
    }
  } else {
    Pause(game, FIRE_PAUSE);
  }
  opeu_music_command7(game, svc_sound);
}

// frame: the routine's frame; the callees run below it.
void opeu_battle_ranged_fire(Game& game, uint16_t shooter, uint16_t target) {
  auto&   data{ Data(game) };
  uint8_t x;
  uint8_t y;
  TargetCell(data, shooter, target, x, y);
  opeu_battle_ranged_panel(game, shooter, target);
  opeu_battle_ranged_tracers(game, shooter, target);
  auto const  shot{ opeu_battle_ranged_damage(game, shooter, target) };
  auto const  losses{ shot.losses };
  auto const  animation{ losses ? HIT_ANIMATION : MISS_ANIMATION };
  auto const& type{ ElementTypeOf(data, shooter) };
  if (FiresShells(type)) {
    uint8_t heavy{ 0 };
    if (target) heavy = IsOfCategory(ElementTypeOf(data, target), Category::INFANTRY) ? losses >= 30 : losses >= 10;
    opeu_battle_animate_element(game, x, y, heavy, 1, animation, target);
  } else if (type.category <= Bits(Category::SUPPLY)) {
    opeu_battle_animate_element(game, x, y, 0, 0, animation, target);
  }
  opeu_battle_panel_begin(game);
  data.battle_element_destroyed = 0;
  if (!losses) {
    opeu_battle_refresh_cell(game, x, y);
  } else if (!shot.into_bunker) {
    auto&      hit{ Record<Element>(data, target) };
    auto const before{ hit.quantity };
    uint16_t   quantity{ before };
    opeu_sub_word(losses, &quantity);
    hit.quantity = quantity;
    opeu_battle_spend_fuel(game, target, before);
    opeu_battle_spend_ammo(game, target, before);
    opeu_battle_spend_stores(game, target, before);
    if (!hit.quantity) {
      hit.damage = 0;
      opeu_battle_draw_cell(game, x, y);
      opeu_battle_wiped_notice(game, target);
      data.battle_element_destroyed = 1;
      auto& own{ Record<Element>(data, shooter) };
      opeu_add_byte(1, RATING_LIMIT, &own.training);
      opeu_add_byte(3, RATING_LIMIT, &own.morale);
    } else {
      // Vehicles take damage by the crew's skill.
      if (auto const category{ ElementTypeOf(data, target).category }) {
        auto const roll{ opeu_rand_bounded(300) };
        auto const skill{ opeu_battle_type_skill(game, target) };
        uint16_t   damage{ hit.damage };
        opeu_add_word(static_cast<uint16_t>(static_cast<uint16_t>(skill * losses) / static_cast<uint16_t>(700 - roll)), data.request_capacities[category], &damage);
        hit.damage = damage;
      }
      opeu_battle_redraw_cell(game, x, y, target, opeu_battle_mode(game, target));
    }
  } else {
    // A shot into a bunker: the garrison share there and the objective's strength take the losses.
    auto& objective{ Record<Objective>(data, opeu_find_objective_at(game, data.battle_x, data.battle_y)) };
    for (unsigned i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i) {
      auto& extra{ BattleExtraAt(data, i) };
      if (extra.x == BATTLE_UNPLACED) break;
      if (extra.x == x && extra.y == y) {
        opeu_sub_byte(losses, &extra.count);
        opeu_sub_byte(losses, &objective.strength);
        opeu_battle_refresh_cell(game, x, y);
        break;
      }
    }
  }
  opeu_battle_set_order(game, shooter, 0);
}
