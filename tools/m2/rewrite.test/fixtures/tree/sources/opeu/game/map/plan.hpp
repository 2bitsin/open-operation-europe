// The fixture's plan services: a dispatcher whose cases read the words through local helpers, and the routines they call.
#pragma once

#include <cstdint>

enum class PlanService : std::uint16_t {
  DRAW_FAR,
  DRAW_NEAR,
  COMPOSE,
  HIGHLIGHT_VIEW,
  HIGHLIGHT_OVERVIEW,
};

enum class CellService : std::uint16_t {
  FLASH = 0x10,
};

auto opeu_draw_far(std::uint16_t x, std::uint16_t y, std::uint8_t* image) -> std::uint16_t;
void opeu_draw_near(std::uint16_t x, std::uint16_t y, std::uint8_t* buffer);
void opeu_compose(std::uint8_t* image, std::uint8_t* mask, std::uint16_t width);
void opeu_highlight_view(std::uint8_t* ds, std::uint8_t x);
void opeu_highlight_overview(std::uint8_t* ds, std::uint8_t x);
auto opeu_next_column(std::uint8_t* ds) -> std::uint16_t;
