// The routine link table (game/link/link.cpp, gen/link-table.cpp): routine identifier (the routine's original address)
// -> the handler that calls it, for the address-keyed service callbacks of the battle modules and the far code
// pointers in the data. M2's batch B4 replaces it with direct calls and deletes it.
#pragma once

#include "common/events.hpp"

#include <cstdint>

/* A linked routine. Arguments are 16-bit words in the address-keyed
 * convention: register arguments in AX, DX, BX order (bytes zero-extended),
 * then stack arguments in prototype order; far pointers are (offset,
 * segment), 32-bit values (low, high). Near pointers are DS offsets. The
 * result is DX:AX (AX in the low half). */
using PortRoutine = std::uint32_t (*)(Game& game, std::uint16_t const* args, unsigned count);

struct PortLink {
  std::uint32_t address; /* original linear routine address (identifier only) */
  char const*   name;
  PortRoutine   routine; /* nullptr: not linked */
};

/* The signature of a linked routine handler port_routine_<name>;
 * gen/link-table.cpp declares every one with it and lists it under its
 * routine address. */
using PortHandler = auto(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t;

/* Call a routine by its identifier; fatal (with its name) when unlinked. */
auto port_call(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count) -> std::uint32_t;
/* The address-keyed service callback signature used by the battle modules. */
auto port_service(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count) -> std::uint16_t;
/* Name of a routine identifier (for diagnostics), or nullptr. */
auto port_routine_name(std::uint32_t address) -> char const*;
/* Identifier of a routine by name (the DOS symbol without _ / @ prefix), 0 if unknown. */
auto port_routine_address(char const* name) -> std::uint32_t;
