// The fixture's link table declarations.
#pragma once

#include "common/events.hpp"

#include <cstdint>

using PortHandler = auto(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t;

/* Call a routine by its identifier. */
auto port_call(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count) -> std::uint32_t;
/* The address-keyed service callback of the battle modules. */
auto port_service(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count) -> std::uint16_t;
