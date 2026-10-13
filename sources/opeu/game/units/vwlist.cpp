// The information views' list builders and the row checks of their list dialogs.
#include "units/vwlist.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/view-service.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "link/glue.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace { } // namespace

// Lists the named objectives `side` holds in view_objectives; returns their count.
uint8_t opeu_list_side_objectives(Game& game, HostEvent event, uint8_t side) {
  auto&        data{ Data(game) };
  std::uint8_t count{ 0 };
  for (auto const& objective : data.objectives)
    if (objective.active && static_cast<std::uint8_t>(Call(game, event, ViewService::OBJECTIVE_SIDE, Near(data, objective))) == side) data.view_objectives[count++] = Near(data, objective);
  return count;
}

// Lists a regiment's elements in the element rows; returns their count.
uint8_t opeu_list_unit_elements(Game& game, HostEvent event, uint16_t unit) {
  (void)event;
  auto&        data{ Data(game) };
  std::uint8_t count{ 0 };
  auto const   chain{ Elements(data, Record<Unit>(data, unit)) };
  for (auto it{ chain.begin() }; it != chain.end(); ++it) data.row_elements[count++] = it.Near();
  return count;
}

// Lists an objective's stock items that hold an amount in view_stocks; returns their count.
uint8_t opeu_list_objective_stocks(Game& game, HostEvent event, uint16_t objective) {
  (void)event;
  auto&        data{ Data(game) };
  std::uint8_t count{ 0 };
  for (auto const& item : Record<Objective>(data, objective).items)
    if (item.quantity) data.view_stocks[count++] = Near(data, item);
  return count;
}

// The total quantity of a regiment's spotted elements whose type is of `category`; their number goes to the byte at
// `count`.
uint16_t opeu_element_kind_total(Game& game, HostEvent event, uint16_t unit, uint8_t category, uint16_t count) {
  (void)event;
  auto&         data{ Data(game) };
  std::uint16_t total{ 0 };
  Byte(game, count) = 0;
  for (auto const& element : Elements(data, Record<Unit>(data, unit)))
    if (data.element_types[element.type].category == category && (element.flags & Bits(ElementFlag::SPOTTED))) {
      total += element.quantity;
      ++Byte(game, count);
    }
  return total;
}

// Row check: an own objective that may be listed, or an enemy objective that was reconnoitred.
uint8_t opeu_view_objective_ok(Game& game, HostEvent event) {
  auto&      data{ Data(game) };
  auto const near{ data.view_objectives[data.list_row] };
  auto const side{ static_cast<std::uint8_t>(Call(game, event, ViewService::OBJECTIVE_SIDE, near)) };
  return (side == data.current_side && Call(game, event, ViewService::OBJECTIVE_ELIGIBLE, near)) || (side != data.current_side && (Record<Objective>(data, near).flags & Bits(ObjectiveFlag::MARKED)));
}

// Row check that rejects every row.
uint8_t opeu_view_false(Game& game, HostEvent event) {
  (void)game;
  (void)event;
  return 0;
}

// Row check: an active division whose selected regiment may be selected.
uint8_t opeu_view_group_ok(Game& game, HostEvent event) {
  auto&       data{ Data(game) };
  auto const& division{ GroupOf(data, data.current_side, data.list_row) };
  return (division.flags & Bits(GroupFlag::READY)) && Call(game, event, ViewService::UNIT_ELIGIBLE, Call(game, event, ViewService::GROUP_SELECTED_UNIT, Near(data, division)));
}

// Row check: a ready regiment, selectable if it is ours, spotted if it is the enemy's.
uint8_t opeu_view_unit_ok(Game& game, HostEvent event) {
  auto&       data{ Data(game) };
  auto const  unit{ data.row_units[data.list_row] };
  auto const& regiment{ Record<Unit>(data, unit) };
  auto const  side{ static_cast<std::uint8_t>(Call(game, event, ViewService::UNIT_SIDE, unit)) };
  return (regiment.flags & Bits(UnitFlag::READY))
         && ((side == data.current_side && Call(game, event, ViewService::UNIT_ELIGIBLE, unit)) || (side != data.current_side && (regiment.flags & Bits(UnitFlag::SPOTTED))));
}

// Further row checks that reject every row.
uint8_t opeu_view_false_2(Game& game, HostEvent event) {
  return opeu_view_false(game, event);
}

uint8_t opeu_view_false_3(Game& game, HostEvent event) {
  return opeu_view_false(game, event);
}

// Row check that accepts every row.
uint8_t opeu_view_true(Game& game, HostEvent event) {
  (void)game;
  (void)event;
  return 1;
}

// Appends to the word list at `list` the equipment types of `category` that a nation in `mask` fields and that are in
// service by the current month (the type's date is year * 16 + month); returns how many were appended.
uint8_t opeu_list_element_types(Game& game, HostEvent event, uint16_t list, uint8_t mask, uint8_t category) {
  (void)event;
  auto& data{ Data(game) };
  auto  at{ list };
  for (auto const& type : data.element_types)
    if ((type.sides & mask) && type.category == category && (data.year << 4) + data.month >= type.date) {
      SetWord(game, at, Near(data, type));
      at = static_cast<std::uint16_t>(at + 2);
    }
  return static_cast<std::uint8_t>(static_cast<std::int16_t>(static_cast<std::uint16_t>(at - list)) >> 1);
}

// The views' calls this module makes.
uint16_t svc_vwlist(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<ViewService>(id)) {
  case ViewService::OBJECTIVE_SIDE:      r = opeu_objective_side(game, Arg(args, count, 0)); break;
  case ViewService::OBJECTIVE_ELIGIBLE:  r = opeu_objective_eligible(game, Arg(args, count, 0)); break;
  case ViewService::GROUP_SELECTED_UNIT: r = opeu_find_group_selected_unit(game, Arg(args, count, 0)); break;
  case ViewService::UNIT_ELIGIBLE:       r = opeu_unit_selection_eligible(game, Arg(args, count, 0)); break;
  case ViewService::UNIT_SIDE:           r = opeu_unit_side(Arg(args, count, 0)); break;
  default:                               port_unhandled("vwlist", id, count);
  }
  return r;
}

auto port_routine_opeu_view_false(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_false(game, svc_vwlist);
}

auto port_routine_opeu_view_false_2(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_false_2(game, svc_vwlist);
}

auto port_routine_opeu_view_false_3(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_false_3(game, svc_vwlist);
}

auto port_routine_opeu_view_group_ok(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_group_ok(game, svc_vwlist);
}

auto port_routine_opeu_view_objective_ok(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_objective_ok(game, svc_vwlist);
}

auto port_routine_opeu_view_true(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_true(game, svc_vwlist);
}

auto port_routine_opeu_view_unit_ok(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_view_unit_ok(game, svc_vwlist);
}
