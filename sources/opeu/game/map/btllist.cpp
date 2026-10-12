// The side of a battle element and of a battle unit, the battle unit owning an element and a unit's element count.
#include "map/btllist.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

uint8_t opeu_battle_element_side(uint8_t* ds, BattleEvent, uint16_t element) {
  return BattleSide(Record<Element>(Data(ds), element));
}

uint8_t opeu_battle_unit_side(uint8_t* ds, BattleEvent, uint16_t unit) {
  auto& data{ Data(ds) };
  return BattleSide(Record<Element>(data, Record<Unit>(data, unit).first));
}

uint16_t opeu_battle_element_unit(uint8_t* ds, BattleEvent, uint16_t element) {
  auto& data{ Data(ds) };
  for (unsigned side{ 0 }; side < SIDE_COUNT; ++side)
    for (unsigned i{ 0 }; i < BATTLE_LIST_LENGTH; ++i)
      if (auto const unit{ BattleList(data, side, i) })
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
          if (it.Near() == element) return unit;
  return 0;
}

uint8_t opeu_battle_element_count(uint8_t* ds, BattleEvent, uint16_t unit) {
  auto&   data{ Data(ds) };
  uint8_t count{ 0 };
  for ([[maybe_unused]] auto const& element : Elements(data, Record<Unit>(data, unit))) ++count;
  return count;
}

uint16_t svc_btllist(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_element_count(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_element_count(ds, svc_btllist, Arg(args, count, 0));
}

auto port_routine_opeu_battle_element_side(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_element_side(ds, svc_btllist, Arg(args, count, 0));
}

auto port_routine_opeu_battle_element_unit(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_battle_element_unit(ds, svc_btllist, Arg(args, count, 0));
}

auto port_routine_opeu_battle_unit_side(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_unit_side(ds, svc_btllist, Arg(args, count, 0));
}
