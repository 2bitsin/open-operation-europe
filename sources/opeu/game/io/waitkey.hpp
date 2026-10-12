// Timed waits that poll the keyboard.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_delay_ticks(std::uint8_t* ds, HostEvent event, std::uint16_t ticks);
void opeu_delay_units(std::uint8_t* ds, HostEvent event, std::uint8_t units);
void opeu_pending_finish(std::uint8_t* ds, HostEvent event, std::uint8_t value);

// The module's services: the direct calls behind its events.
auto svc_waitkey(std::uint8_t* ds, std::uint16_t kind, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t;
