// The fixture's battle routines.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto     opeu_grid_step(std::uint8_t* ds, std::uint16_t a, std::uint16_t b) -> std::uint8_t;
void     opeu_set_order(std::uint8_t* ds, BattleEvent event, std::uint16_t element, std::uint16_t order);
auto     opeu_path_step(std::uint8_t* ds, std::uint16_t x, std::uint16_t y) -> std::uint16_t;
auto     opeu_path_mark(std::uint8_t* ds, std::uint16_t x) -> std::uint16_t;
auto     opeu_ask(std::uint8_t* ds, std::uint16_t question) -> bool;
void     opeu_mark_cell(std::uint8_t* ds, std::uint16_t cell);
uint16_t svc_btlwalk(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned count);
