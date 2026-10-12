// The row-draw and choice callbacks of the saved-game list.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_tbl_save_accept() -> std::uint8_t;
void opeu_tbl_draw_saves(std::uint8_t* ds, HostEvent event, std::uint16_t scratch, std::uint8_t column, std::uint8_t first, std::uint8_t row, std::uint8_t last, std::uint8_t flags);

// The module's services: the direct calls behind its events.
auto svc_tblsave(std::uint8_t* ds, std::uint16_t kind, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t;
