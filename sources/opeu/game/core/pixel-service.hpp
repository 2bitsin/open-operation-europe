// A map cell's screen pixel from a module's pixel service (map/mappixel's converters behind its event), which converts
// coordinate words in the data segment: they live in a ServiceLocals reserve for the call.
#pragma once

#include "common/events.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"

#include <cstdint>

namespace opeu::game::detail::pixel_service {

// The pixel of the cell (x, y) as the service converts it.
template <typename Service>
auto ServicePixel(std::uint8_t* ds, HostEvent event, Service service, std::uint16_t x, std::uint16_t y) -> Pixel {
  constexpr std::uint16_t Y_AT{ 0 };
  constexpr std::uint16_t X_AT{ 2 };
  ServiceLocals const     locals{ 4 };
  SetWord(ds, locals.At(X_AT), x);
  SetWord(ds, locals.At(Y_AT), y);
  Call(ds, event, service, locals.At(X_AT), locals.At(Y_AT));
  return { Word(ds, locals.At(X_AT)), Word(ds, locals.At(Y_AT)) };
}
} // namespace opeu::game::detail::pixel_service

namespace opeu::game {
using detail::pixel_service::ServicePixel;
} // namespace opeu::game
