// The five-digit numeric entry dialog with a slider bar.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_numdlg_button(Game& game, std::uint8_t n, std::uint16_t tile);
void opeu_numdlg_press(Game& game, std::uint8_t n);
void opeu_numdlg_release(Game& game, std::uint8_t n);
void opeu_numdlg_open(Game& game);
void opeu_numdlg_close(Game& game);
void opeu_numdlg_digit(Game& game, std::uint8_t i, std::uint8_t digit);
void opeu_numdlg_split(Game& game, std::int32_t value);
auto opeu_numdlg_value(Game& game) -> std::int32_t;
void opeu_numdlg_show(Game& game, std::int32_t value);
void opeu_numdlg_bar(Game& game, std::uint16_t lo, std::uint16_t hi, std::int32_t value);
auto opeu_numdlg_hit(Game& game, std::uint16_t mx, std::uint16_t my) -> std::uint8_t;
void opeu_numdlg_highlight(Game& game, std::uint8_t n);
void opeu_numdlg_drag(Game& game, HostService service);
void opeu_numdlg_refresh(Game& game, std::uint8_t n);
auto opeu_numdlg_increment(Game& game, std::uint8_t n, std::uint16_t lo, std::uint16_t hi, std::int32_t value) -> std::int32_t;
auto opeu_numdlg_decrement(Game& game, std::uint8_t n, std::uint16_t lo, std::uint16_t hi, std::int32_t value) -> std::int32_t;
auto opeu_numdlg_outside(Game& game, std::uint16_t mx, std::uint16_t my) -> std::uint16_t;
auto opeu_numdlg_track(Game& game, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel) -> std::uint16_t;
auto opeu_numdlg_run(Game& game, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel) -> std::uint16_t;

// The module's services: the shared input calls it raises.
auto svc_numdlg(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
