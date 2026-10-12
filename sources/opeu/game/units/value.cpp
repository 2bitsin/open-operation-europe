// Equipment-type category classifiers, the hour-of-day classes and the combat value of types, elements and units.
#include "units/value.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

uint16_t opeu_category_0_to_3(uint8_t code) {
  return code <= 3;
}
uint16_t opeu_category_4_or_5(uint8_t code) {
  return code == 4 || code == 5;
}
uint16_t opeu_category_6(uint8_t code) {
  return code == 6;
}
uint16_t opeu_category_7(uint8_t code) {
  return code == 7;
}
uint16_t opeu_category_0_to_5(uint8_t code) {
  return code <= 5;
}

// The combat value of one item of an equipment type.
uint16_t opeu_type_value(const uint8_t ds[65536], uint16_t type) {
  auto const& record{ Record<ElementType>(Data(ds), type) };
  auto const  heavy{ (record.flags & Bits(TypeFlag::HEAVY)) != 0 };
  switch (record.category) {
  case 0: return 1;
  case 1: return 10;
  case 2: return 30;
  case 3:
    if (record.flags & Bits(TypeFlag::LIGHT)) return 10;
    return heavy ? 100 : 50;
  case 4:  return heavy ? 100 : 30;
  case 5:  return 3;
  case 6:
  case 7:  return 2;
  default: return 0;
  }
}

// The value of an element: its type's value per started ten items.
uint16_t opeu_element_value(const uint8_t ds[65536], uint16_t element) {
  auto const& data{ Data(ds) };
  auto const& record{ Record<Element>(data, element) };
  return opeu_type_value(ds, Near(data, data.element_types[record.type])) * (record.quantity / 10 + 1);
}

// The value of a unit: the sum over its elements.
uint16_t opeu_unit_value(const uint8_t ds[65536], uint16_t unit) {
  auto const&   data{ Data(ds) };
  std::uint16_t value{ 0 };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    value += opeu_element_value(ds, it.Near());
  return value;
}

uint16_t opeu_hour_is_dawn(uint8_t code) {
  return code >= 5 && code <= 7;
}
uint16_t opeu_hour_is_day(uint8_t code) {
  return code >= 8 && code <= 16;
}
uint16_t opeu_hour_is_dusk(uint8_t code) {
  return code >= 17 && code <= 19;
}
uint16_t opeu_hour_is_night(uint8_t code) {
  return code >= 20 || code <= 4;
}

auto port_routine_opeu_category_6(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_category_6((uint8_t)Arg(args, count, 0));
}

auto port_routine_opeu_category_7(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_category_7((uint8_t)Arg(args, count, 0));
}

auto port_routine_opeu_hour_is_night(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_hour_is_night((uint8_t)Arg(args, count, 0));
}
