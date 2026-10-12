// Loads a scenario's text, the message pictures and the scenario's map, frame, tile and font resources.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_load_scenario_text(std::uint8_t* ds, HostEvent event,
                             std::uint8_t number, std::uint16_t destination);
void opeu_load_message_graphics(std::uint8_t* ds, HostEvent event);
void opeu_load_scenario_graphics(std::uint8_t* ds, HostEvent event);

// The module's services: the direct calls behind its events.
auto svc_scenres(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args,
                 std::uint16_t count) -> std::uint16_t;
