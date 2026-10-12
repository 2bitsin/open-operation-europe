// Battle accessors: an element's position record, the battle cells and the group's level against the round.
#include "map/btlunit.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "map/btllist.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t STRENGTH_LOW{ 0x0f };
constexpr uint8_t MODE_LAST{ 3 };
// The order level of a group: no order, an order the group started (progress 200) or a fresh one, and otherwise.
constexpr uint8_t NO_ORDER{ 0xff };
constexpr uint8_t ORDER_STARTED{ 200 };
constexpr uint8_t LEVEL_NO_ORDER{ 1 };
constexpr uint8_t LEVEL_UNDER_WAY{ 2 };
constexpr uint8_t LEVEL_TOP{ 6 };

[[nodiscard]] auto Position(uint8_t* ds, uint16_t element) -> BattlePosition& {
  return PositionOf(Data(ds), element);
}
[[nodiscard]] auto Cell(uint8_t* ds, uint8_t x, uint8_t y) -> uint8_t& {
  return BattleCellAt(Data(ds), x, y);
}
[[nodiscard]] auto Occupant(uint8_t* ds, uint8_t x, uint8_t y) -> uint8_t {
  return Cell(ds, x, y) & Bits(BattleCell::OCCUPANT_MASK);
}
} // namespace

uint8_t opeu_battle_state(uint8_t* ds, BattleEvent, uint16_t element) {
  return Position(ds, element).state & Bits(BattleState::STATE_MASK);
}

uint8_t opeu_battle_order(uint8_t* ds, BattleEvent, uint16_t element) {
  return Position(ds, element).order;
}

void opeu_battle_set_order(uint8_t* ds, BattleEvent, uint16_t element, uint8_t order) {
  Position(ds, element).order = order;
}

// side: the side whose element stands on the cell.
void opeu_battle_mark_cell(uint8_t* ds, BattleEvent, uint8_t x, uint8_t y, uint8_t side) {
  auto& cell{ Cell(ds, x, y) };
  cell = static_cast<uint8_t>((cell & ~Bits(BattleCell::OCCUPANT_MASK)) | static_cast<uint8_t>(side + 1));
}

void opeu_battle_clear_cell(uint8_t* ds, BattleEvent, uint8_t x, uint8_t y) {
  Cell(ds, x, y) &= static_cast<uint8_t>(~Bits(BattleCell::OCCUPANT_MASK));
}

uint16_t opeu_battle_element_flag2(uint8_t* ds, BattleEvent, uint16_t element) {
  return (Record<Element>(Data(ds), element).flags & Bits(ElementFlag::SPOTTED)) ? 1 : 0;
}

// An element takes part while it is on the field and linked into its unit.
uint16_t opeu_battle_element_active(uint8_t* ds, BattleEvent, uint16_t element) {
  if ((Position(ds, element).x & BATTLE_LEFT) || !(Record<Element>(Data(ds), element).flags & Bits(ElementFlag::READY)))
    return 0;
  return 1;
}

uint16_t opeu_battle_unit_active(uint8_t* ds, BattleEvent event, uint16_t unit) {
  auto& data{ Data(ds) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (opeu_battle_element_active(ds, event, it.Near())) return 1;
  return 0;
}

void opeu_battle_set_mode(uint8_t* ds, BattleEvent, uint16_t element, uint8_t mode) {
  if (mode <= MODE_LAST) Position(ds, element).mode = mode;
}

uint8_t opeu_battle_mode(uint8_t* ds, BattleEvent, uint16_t element) {
  return element ? Position(ds, element).mode : 0;
}

uint16_t opeu_battle_engaged(uint8_t* ds, BattleEvent, uint16_t element) {
  return (Position(ds, element).state & Bits(BattleState::ENGAGED)) ? 1 : 0;
}

void opeu_battle_engage(uint8_t* ds, BattleEvent, uint16_t element) {
  Position(ds, element).state |= Bits(BattleState::ENGAGED);
}

void opeu_battle_disengage(uint8_t* ds, BattleEvent, uint16_t element) {
  Position(ds, element).state &= Bits(BattleState::STATE_MASK);
}

uint8_t opeu_battle_facing(uint8_t* ds, BattleEvent, uint16_t element) {
  return Position(ds, element).facing;
}

void opeu_battle_set_facing(uint8_t* ds, BattleEvent, uint16_t element, uint8_t facing) {
  Position(ds, element).facing = facing;
}

uint8_t opeu_battle_strength_low(uint8_t* ds, BattleEvent, uint16_t element) {
  return Position(ds, element).strength & STRENGTH_LOW;
}

uint8_t opeu_battle_strength_high(uint8_t* ds, BattleEvent, uint16_t element) {
  return Position(ds, element).strength >> 4;
}

void opeu_battle_set_strength(uint8_t* ds, BattleEvent, uint16_t element, uint8_t low, uint8_t high) {
  auto& position{ Position(ds, element) };
  position.strength  = low;
  position.strength |= static_cast<uint8_t>(high << 4);
}

uint16_t opeu_battle_cell_occupied(uint8_t* ds, BattleEvent, uint8_t x, uint8_t y) {
  return Occupant(ds, x, y) ? 1 : 0;
}

// side: the side + 1 of the occupant asked for (0 for an empty cell).
uint16_t opeu_battle_cell_is(uint8_t* ds, BattleEvent, uint8_t x, uint8_t y, uint8_t side) {
  return Occupant(ds, x, y) == side ? 1 : 0;
}

// The level of the element's unit's order: what the battle_levels table gives its order kind (read past the table
// for the later kinds, as the original does).
uint8_t opeu_battle_group_level(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&       data{ Data(ds) };
  auto const& order{ Record<Unit>(data, opeu_battle_element_unit(ds, event, element)).orders[0] };
  if (order.kind == NO_ORDER) return LEVEL_NO_ORDER;
  if (order.progress == ORDER_STARTED) return data.battle_levels[0];
  if (order.progress == 0)
    return Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_levels) + order.kind));
  return LEVEL_UNDER_WAY;
}

// The round from which the element's group joins the fight.
uint8_t opeu_battle_level_gap(uint8_t* ds, BattleEvent event, uint16_t element) {
  return static_cast<uint8_t>(LEVEL_TOP - opeu_min16(opeu_battle_group_level(ds, event, element), LEVEL_TOP));
}

uint16_t opeu_battle_level_allowed(uint8_t* ds, BattleEvent event, uint16_t element) {
  return opeu_battle_level_gap(ds, event, element) <= Data(ds).battle_level_limit ? 1 : 0;
}

uint16_t svc_btlunit(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_cell_occupied(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_cell_occupied(ds, svc_btlunit, static_cast<uint8_t>(Arg(args, count, 0)),
                                   static_cast<uint8_t>(Arg(args, count, 1)));
}

auto port_routine_opeu_battle_clear_cell(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_clear_cell(ds, svc_btlunit, static_cast<uint8_t>(Arg(args, count, 0)),
                         static_cast<uint8_t>(Arg(args, count, 1)));
  return 0;
}

auto port_routine_opeu_battle_element_active(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_element_active(ds, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_element_flag2(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_element_flag2(ds, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_level_allowed(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_level_allowed(ds, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_mode(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_mode(ds, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_order(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_order(ds, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_set_order(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_set_order(ds, svc_btlunit, Arg(args, count, 1), static_cast<uint8_t>(Arg(args, count, 0)));
  return 0;
}

auto port_routine_opeu_battle_unit_active(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_unit_active(ds, svc_btlunit, Arg(args, count, 0));
}
