// The fixture's callback types.
#pragma once

#include <cstdint>

using HostEvent   = std::uint16_t (*)(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
using BattleEvent = std::uint16_t (*)(std::uint8_t* ds, std::uint32_t service, std::uint16_t const* args, unsigned count);
