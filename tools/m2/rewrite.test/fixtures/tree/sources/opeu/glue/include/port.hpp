// The fixture's glue: word reads and the link table calls.
#pragma once

#include <cstdint>

using PortHandler = auto(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t;

inline auto Arg(std::uint16_t const* args, unsigned count, unsigned i) -> std::uint16_t {
  return i < count ? args[i] : 0;
}

auto port_call(std::uint8_t* ds, std::uint32_t address, std::uint16_t const* args, unsigned count) -> std::uint32_t;
auto port_service(std::uint8_t* ds, std::uint32_t address, std::uint16_t const* args, unsigned count) -> std::uint16_t;
auto btl_reorder(std::uint8_t* ds, std::uint32_t address, std::uint16_t const* args, unsigned count, unsigned char const* order, unsigned n) -> std::uint16_t;
auto port_dialogs_host() -> void*;
