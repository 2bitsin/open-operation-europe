// The fixture's link table lookup.
#include "link/link.hpp"

std::uint16_t port_service(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count) {
  return static_cast<std::uint16_t>(port_call(game, address, args, count));
}
