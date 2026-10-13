// The fixture's forwarding dispatchers: services one dispatcher hands on to another as they are, through an out word,
// assigned, or remapped through a table, and a dispatcher that forwards unless a predicate on the id holds.
#pragma once

#include "common/events.hpp"

#include <cstdint>

enum OrderService : std::uint16_t {
  UNIT_VALUE = 0,
  UNIT_SIDE  = 1,
  WAIT_UNITS = 2,
  SOUND_CUE  = 3,
};

enum UnitService : std::uint16_t {
  VALUE    = 0,
  QUANTITY = 1,
  SIDE     = 2,
  REPORT   = 3,
  SERVICES = 4,
};

auto     opeu_unit_value(std::uint8_t* ds, std::uint16_t unit) -> std::uint16_t;
auto     opeu_unit_side(std::uint16_t unit) -> std::uint8_t;
void     opeu_wait_units(std::uint8_t* ds, std::uint8_t units);
void     opeu_sound_cue(std::uint8_t* ds);
auto     opeu_unit_quantity(std::uint8_t* ds, std::uint16_t unit) -> std::uint16_t;
int      svc_order_handler(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result);
uint16_t svc_unit_orders(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
