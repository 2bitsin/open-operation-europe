// Battle losses of commanders. When the element carrying a unit's commander is destroyed, the loss is reported and,
// for the division's selected unit, the division's general (and, when the division is the side's selected one, the
// side's staff of kinds 1, 3 and 2) may be hit: a draw under 10 hits a general not already out (unless the staff
// trait saves him on a second draw), under 5 fatally (the replacement deputy is appointed), otherwise he is wounded.
#include "map/btldepty.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t HIT_BELOW{ 10 };
constexpr uint8_t FATAL_BELOW{ 5 };

[[nodiscard]] auto Draw(Game& game, BattleEvent event) -> uint8_t {
  return static_cast<uint8_t>(Call(game, event, BattleRoutine::RAND_BOUNDED, 100));
}

// One general's draw; fatal: a fatal hit marks him out instead of appointing the deputy.
void DrawLoss(Game& game, BattleEvent event, uint16_t person, uint8_t draw, bool fatal) {
  if (draw >= HIT_BELOW || !person) return;
  auto& role{ Record<Person>(Data(game), person).role };
  if (role & Bits(PersonRole::DEPUTY)) return;
  if ((role & Bits(PersonRole::STAFF_TRAIT)) && Draw(game, event) < 50) return;
  if (draw >= FATAL_BELOW)
    role |= Bits(PersonRole::POOR_HEALTH);
  else if (fatal)
    role |= Bits(PersonRole::DEPUTY);
  else
    Call(game, event, BattleRoutine::INITIALIZE_DEPUTY, person);
}
} // namespace

void opeu_battle_commander_lost(Game& game, BattleEvent event) {
  auto&      data{ Data(game) };
  auto const unit{ data.battle_lost_unit };
  if (unit == Near(data, data.battle_garrison)) return;
  auto const side{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_UNIT_SIDE, unit)) };
  auto const group{ opeu_unit_group_index(unit) };
  auto       draw{ Draw(game, event) };
  DrawLoss(game, event, draw < HIT_BELOW ? opeu_find_group_record(game, side, group) : 0, draw, false);
  if (opeu_unit_group_index(opeu_find_side_selected_unit(game, side)) != group) return;
  draw = Draw(game, event);
  DrawLoss(game, event, draw < HIT_BELOW ? opeu_find_side_record(game, side, 1) : 0, draw, true);
  draw = Draw(game, event);
  DrawLoss(game, event, draw < HIT_BELOW ? opeu_find_side_record(game, side, 3) : 0, draw, false);
  draw = Draw(game, event);
  DrawLoss(game, event, draw < HIT_BELOW ? opeu_find_side_record(game, side, 2) : 0, draw, false);
}

void opeu_battle_report_loss(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  if (data.battle_lost_element) Call(game, event, BattleRoutine::BATTLE_AUTO_HQ);
  if (Record<Unit>(data, data.battle_lost_unit).flags & Bits(UnitFlag::SELECTED)) opeu_battle_commander_lost(game, event);
}

void opeu_battle_element_lost(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  auto const unit{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_UNIT, element) };
  bool const lead{ (Record<Element>(data, element).flags & Bits(ElementFlag::SELECTED)) != 0 };
  Call(game, event, BattleRoutine::BATTLE_REMOVE_UNIT, element, unit);
  auto& record{ Record<Unit>(data, unit) };
  if (!(record.flags & Bits(UnitFlag::READY))) {
    record.resources[1] = 0;
    record.resources[0] = 0;
    record.resources[2] = 0;
    Call(game, event, BattleRoutine::BATTLE_REMOVE_LIST, unit);
    return;
  }
  if (lead && unit != Near(data, data.battle_garrison)) {
    data.battle_lost_element = element;
    data.battle_lost_unit    = unit;
    opeu_battle_report_loss(game, event);
    data.battle_lost_element = 0;
    data.battle_lost_unit    = 0;
  }
}

uint16_t svc_btldepty(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_element_lost(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_element_lost(game, svc_btldepty, Arg(args, count, 0));
  return 0;
}
