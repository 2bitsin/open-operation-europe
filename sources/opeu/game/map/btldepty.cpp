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

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t HIT_BELOW{ 10 };
constexpr uint8_t FATAL_BELOW{ 5 };

[[nodiscard]] auto Draw(uint8_t* ds, BattleEvent event) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::RAND_BOUNDED, 100));
}

// One general's draw; fatal: a fatal hit marks him out instead of appointing the deputy.
void DrawLoss(uint8_t* ds, BattleEvent event, uint16_t person, uint8_t draw, bool fatal) {
  if (draw >= HIT_BELOW || !person) return;
  auto& role{ Record<Person>(Data(ds), person).role };
  if (role & Bits(PersonRole::DEPUTY)) return;
  if ((role & Bits(PersonRole::STAFF_TRAIT)) && Draw(ds, event) < 50) return;
  if (draw >= FATAL_BELOW)
    role |= Bits(PersonRole::POOR_HEALTH);
  else if (fatal)
    role |= Bits(PersonRole::DEPUTY);
  else
    Call(ds, event, BattleRoutine::INITIALIZE_DEPUTY, person);
}
} // namespace

void opeu_battle_commander_lost(uint8_t* ds, BattleEvent event) {
  auto&      data{ Data(ds) };
  auto const unit{ data.battle_lost_unit };
  if (unit == Near(data, data.battle_garrison)) return;
  auto const side{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_SIDE, unit)) };
  auto const group{ opeu_unit_group_index(unit) };
  auto       draw{ Draw(ds, event) };
  DrawLoss(ds, event, draw < HIT_BELOW ? opeu_find_group_record(ds, side, group) : 0, draw, false);
  if (opeu_unit_group_index(opeu_find_side_selected_unit(ds, side)) != group) return;
  draw = Draw(ds, event);
  DrawLoss(ds, event, draw < HIT_BELOW ? opeu_find_side_record(ds, side, 1) : 0, draw, true);
  draw = Draw(ds, event);
  DrawLoss(ds, event, draw < HIT_BELOW ? opeu_find_side_record(ds, side, 3) : 0, draw, false);
  draw = Draw(ds, event);
  DrawLoss(ds, event, draw < HIT_BELOW ? opeu_find_side_record(ds, side, 2) : 0, draw, false);
}

void opeu_battle_report_loss(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  if (data.battle_lost_element) Call(ds, event, BattleRoutine::BATTLE_AUTO_HQ);
  if (Record<Unit>(data, data.battle_lost_unit).flags & Bits(UnitFlag::SELECTED)) opeu_battle_commander_lost(ds, event);
}

void opeu_battle_element_lost(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&      data{ Data(ds) };
  auto const unit{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_UNIT, element) };
  bool const lead{ (Record<Element>(data, element).flags & Bits(ElementFlag::SELECTED)) != 0 };
  Call(ds, event, BattleRoutine::BATTLE_REMOVE_UNIT, element, unit);
  auto& record{ Record<Unit>(data, unit) };
  if (!(record.flags & Bits(UnitFlag::READY))) {
    record.resources[1] = 0;
    record.resources[0] = 0;
    record.resources[2] = 0;
    Call(ds, event, BattleRoutine::BATTLE_REMOVE_LIST, unit);
    return;
  }
  if (lead && unit != Near(data, data.battle_garrison)) {
    data.battle_lost_element = element;
    data.battle_lost_unit    = unit;
    opeu_battle_report_loss(ds, event);
    data.battle_lost_element = 0;
    data.battle_lost_unit    = 0;
  }
}

uint16_t svc_btldepty(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_element_lost(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_battle_element_lost(ds, svc_btldepty, Arg(args, count, 0));
  return 0;
}
