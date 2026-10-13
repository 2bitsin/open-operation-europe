// Battle accessors: an element's position record, the battle cells and the group's level against the round.
#include "map/btlunit.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "map/btllist.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
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

[[nodiscard]] auto Position(Game& game, uint16_t element) -> BattlePosition& {
  return PositionOf(Data(game), element);
}
[[nodiscard]] auto Cell(Game& game, uint8_t x, uint8_t y) -> uint8_t& {
  return BattleCellAt(Data(game), x, y);
}
[[nodiscard]] auto Occupant(Game& game, uint8_t x, uint8_t y) -> uint8_t {
  return Cell(game, x, y) & Bits(BattleCell::OCCUPANT_MASK);
}
} // namespace

uint8_t opeu_battle_state(Game& game, BattleEvent, uint16_t element) {
  return Position(game, element).state & Bits(BattleState::STATE_MASK);
}

uint8_t opeu_battle_order(Game& game, BattleEvent, uint16_t element) {
  return Position(game, element).order;
}

void opeu_battle_set_order(Game& game, BattleEvent, uint16_t element, uint8_t order) {
  Position(game, element).order = order;
}

// side: the side whose element stands on the cell.
void opeu_battle_mark_cell(Game& game, BattleEvent, uint8_t x, uint8_t y, uint8_t side) {
  auto& cell{ Cell(game, x, y) };
  cell = static_cast<uint8_t>((cell & ~Bits(BattleCell::OCCUPANT_MASK)) | static_cast<uint8_t>(side + 1));
}

void opeu_battle_clear_cell(Game& game, BattleEvent, uint8_t x, uint8_t y) {
  Cell(game, x, y) &= static_cast<uint8_t>(~Bits(BattleCell::OCCUPANT_MASK));
}

uint16_t opeu_battle_element_flag2(Game& game, BattleEvent, uint16_t element) {
  return (Record<Element>(Data(game), element).flags & Bits(ElementFlag::SPOTTED)) ? 1 : 0;
}

// An element takes part while it is on the field and linked into its unit.
uint16_t opeu_battle_element_active(Game& game, BattleEvent, uint16_t element) {
  if ((Position(game, element).x & BATTLE_LEFT) || !(Record<Element>(Data(game), element).flags & Bits(ElementFlag::READY))) return 0;
  return 1;
}

uint16_t opeu_battle_unit_active(Game& game, BattleEvent event, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (opeu_battle_element_active(game, event, it.Near())) return 1;
  return 0;
}

void opeu_battle_set_mode(Game& game, BattleEvent, uint16_t element, uint8_t mode) {
  if (mode <= MODE_LAST) Position(game, element).mode = mode;
}

uint8_t opeu_battle_mode(Game& game, BattleEvent, uint16_t element) {
  return element ? Position(game, element).mode : 0;
}

uint16_t opeu_battle_engaged(Game& game, BattleEvent, uint16_t element) {
  return (Position(game, element).state & Bits(BattleState::ENGAGED)) ? 1 : 0;
}

void opeu_battle_engage(Game& game, BattleEvent, uint16_t element) {
  Position(game, element).state |= Bits(BattleState::ENGAGED);
}

void opeu_battle_disengage(Game& game, BattleEvent, uint16_t element) {
  Position(game, element).state &= Bits(BattleState::STATE_MASK);
}

uint8_t opeu_battle_facing(Game& game, BattleEvent, uint16_t element) {
  return Position(game, element).facing;
}

void opeu_battle_set_facing(Game& game, BattleEvent, uint16_t element, uint8_t facing) {
  Position(game, element).facing = facing;
}

uint8_t opeu_battle_strength_low(Game& game, BattleEvent, uint16_t element) {
  return Position(game, element).strength & STRENGTH_LOW;
}

uint8_t opeu_battle_strength_high(Game& game, BattleEvent, uint16_t element) {
  return Position(game, element).strength >> 4;
}

void opeu_battle_set_strength(Game& game, BattleEvent, uint16_t element, uint8_t low, uint8_t high) {
  auto& position{ Position(game, element) };
  position.strength  = low;
  position.strength |= static_cast<uint8_t>(high << 4);
}

uint16_t opeu_battle_cell_occupied(Game& game, BattleEvent, uint8_t x, uint8_t y) {
  return Occupant(game, x, y) ? 1 : 0;
}

// side: the side + 1 of the occupant asked for (0 for an empty cell).
uint16_t opeu_battle_cell_is(Game& game, BattleEvent, uint8_t x, uint8_t y, uint8_t side) {
  return Occupant(game, x, y) == side ? 1 : 0;
}

// The level of the element's unit's order: what the battle_levels table gives its order kind (read past the table
// for the later kinds, as the original does).
uint8_t opeu_battle_group_level(Game& game, BattleEvent event, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& order{ Record<Unit>(data, opeu_battle_element_unit(game, event, element)).orders[0] };
  if (order.kind == NO_ORDER) return LEVEL_NO_ORDER;
  if (order.progress == ORDER_STARTED) return data.battle_levels[0];
  if (order.progress == 0) return Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_levels) + order.kind));
  return LEVEL_UNDER_WAY;
}

// The round from which the element's group joins the fight.
uint8_t opeu_battle_level_gap(Game& game, BattleEvent event, uint16_t element) {
  return static_cast<uint8_t>(LEVEL_TOP - opeu_min16(opeu_battle_group_level(game, event, element), LEVEL_TOP));
}

uint16_t opeu_battle_level_allowed(Game& game, BattleEvent event, uint16_t element) {
  return opeu_battle_level_gap(game, event, element) <= Data(game).battle_level_limit ? 1 : 0;
}

uint16_t svc_btlunit(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_cell_occupied(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_cell_occupied(game, svc_btlunit, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
}

auto port_routine_opeu_battle_clear_cell(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_clear_cell(game, svc_btlunit, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
  return 0;
}

auto port_routine_opeu_battle_element_active(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_element_active(game, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_element_flag2(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_element_flag2(game, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_level_allowed(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_level_allowed(game, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_mode(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_mode(game, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_order(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_order(game, svc_btlunit, Arg(args, count, 0));
}

auto port_routine_opeu_battle_set_order(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_set_order(game, svc_btlunit, Arg(args, count, 1), static_cast<uint8_t>(Arg(args, count, 0)));
  return 0;
}

auto port_routine_opeu_battle_unit_active(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_unit_active(game, svc_btlunit, Arg(args, count, 0));
}
