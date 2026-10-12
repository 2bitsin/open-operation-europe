// The five-digit numeric entry dialog with a slider bar.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_numdlg_button(std::uint8_t* ds, HostService service, std::uint8_t n, std::uint16_t tile);
void opeu_numdlg_press(std::uint8_t* ds, HostService service, std::uint8_t n);
void opeu_numdlg_release(std::uint8_t* ds, HostService service, std::uint8_t n);
void opeu_numdlg_open(std::uint8_t* ds, HostService service);
void opeu_numdlg_close(std::uint8_t* ds, HostService service);
void opeu_numdlg_digit(std::uint8_t* ds, HostService service, std::uint8_t i, std::uint8_t digit);
void opeu_numdlg_split(std::uint8_t* ds, HostService service, std::int32_t value);
auto opeu_numdlg_value(std::uint8_t* ds, HostService service) -> std::int32_t;
void opeu_numdlg_show(std::uint8_t* ds, HostService service, std::int32_t value);
void opeu_numdlg_bar(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, std::int32_t value);
auto opeu_numdlg_hit(std::uint8_t* ds, HostService service, std::uint16_t mx, std::uint16_t my) -> std::uint8_t;
void opeu_numdlg_highlight(std::uint8_t* ds, HostService service, std::uint8_t n);
void opeu_numdlg_drag(std::uint8_t* ds, HostService service, std::uint16_t scratch);
void opeu_numdlg_refresh(std::uint8_t* ds, HostService service, std::uint8_t n);
auto opeu_numdlg_increment(std::uint8_t* ds, HostService service, std::uint8_t n, std::uint16_t lo, std::uint16_t hi,
                           std::int32_t value) -> std::int32_t;
auto opeu_numdlg_decrement(std::uint8_t* ds, HostService service, std::uint8_t n, std::uint16_t lo, std::uint16_t hi,
                           std::int32_t value) -> std::int32_t;
auto opeu_numdlg_outside(std::uint8_t* ds, HostService service, std::uint16_t mx, std::uint16_t my) -> std::uint16_t;
auto opeu_numdlg_track(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel,
                       std::uint16_t scratch) -> std::uint16_t;
auto opeu_numdlg_run(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel,
                     std::uint16_t scratch) -> std::uint16_t;

// The module's services: the shared input calls it raises.
auto svc_numdlg(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
