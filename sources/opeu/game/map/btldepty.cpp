// Battle losses of commanders. When the element carrying a unit's commander is destroyed, the loss is reported and,
// for the division's selected unit, the division's general (and, when the division is the side's selected one, the
// side's staff of kinds 1, 3 and 2) may be hit: a draw under 10 hits a general not already out (unless the staff
// trait saves him on a second draw), under 5 fatally (the replacement deputy is appointed), otherwise he is wounded.
#include "map/btldepty.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/deputy.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/hqloss.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t HIT_BELOW{ 10 };
constexpr uint8_t FATAL_BELOW{ 5 };

[[nodiscard]] auto Draw(Game& game) -> uint8_t {
  return static_cast<uint8_t>(game.State().RandomBounded(100));
}

// One general's draw; fatal: a fatal hit marks him out instead of appointing the deputy.
void DrawLoss(Game& game, uint16_t person, uint8_t draw, bool fatal) {
  if (draw >= HIT_BELOW || !person) return;
  auto& role{ Record<Person>(Data(game), person).role };
  if (role & Bits(PersonRole::DEPUTY)) return;
  if ((role & Bits(PersonRole::STAFF_TRAIT)) && Draw(game) < 50) return;
  if (draw >= FATAL_BELOW)
    role |= Bits(PersonRole::POOR_HEALTH);
  else if (fatal)
    role |= Bits(PersonRole::DEPUTY);
  else
    opeu_initialize_deputy(game, person);
}
} // namespace

void opeu_battle_commander_lost(Game& game) {
  auto&      data{ Data(game) };
  auto const unit{ data.battle_lost_unit };
  if (unit == Near(data, data.battle_garrison)) return;
  auto const side{ static_cast<uint8_t>(opeu_battle_unit_side(game, unit)) };
  auto const group{ opeu_unit_group_index(unit) };
  auto       draw{ Draw(game) };
  DrawLoss(game, draw < HIT_BELOW ? opeu_find_group_record(game, side, group) : 0, draw, false);
  if (opeu_unit_group_index(opeu_find_side_selected_unit(game, side)) != group) return;
  draw = Draw(game);
  DrawLoss(game, draw < HIT_BELOW ? opeu_find_side_record(game, side, 1) : 0, draw, true);
  draw = Draw(game);
  DrawLoss(game, draw < HIT_BELOW ? opeu_find_side_record(game, side, 3) : 0, draw, false);
  draw = Draw(game);
  DrawLoss(game, draw < HIT_BELOW ? opeu_find_side_record(game, side, 2) : 0, draw, false);
}

void opeu_battle_report_loss(Game& game) {
  auto& data{ Data(game) };
  if (data.battle_lost_element) opeu_battle_auto_hq(game);
  if (Record<Unit>(data, data.battle_lost_unit).flags & Bits(UnitFlag::SELECTED)) opeu_battle_commander_lost(game);
}

void opeu_battle_element_lost(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  bool const lead{ (Record<Element>(data, element).flags & Bits(ElementFlag::SELECTED)) != 0 };
  opeu_battle_remove_unit(game, unit, element);
  auto& record{ Record<Unit>(data, unit) };
  if (!(record.flags & Bits(UnitFlag::READY))) {
    record.resources[1] = 0;
    record.resources[0] = 0;
    record.resources[2] = 0;
    opeu_battle_remove_list(game, unit);
    return;
  }
  if (lead && unit != Near(data, data.battle_garrison)) {
    data.battle_lost_element = element;
    data.battle_lost_unit    = unit;
    opeu_battle_report_loss(game);
    data.battle_lost_element = 0;
    data.battle_lost_unit    = 0;
  }
}
