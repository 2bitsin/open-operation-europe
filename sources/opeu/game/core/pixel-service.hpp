// A map cell's pixel in the tactical view, converted by map/mappixel in coordinate words that live in a ServiceLocals
// reserve for the conversion.
#pragma once

#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/mappixel.hpp"

#include <cstdint>

namespace opeu::game::detail::pixel_service {

// The view pixel of the cell (x, y).
inline auto CellViewPixel(std::uint8_t* ds, std::uint16_t x, std::uint16_t y) -> Pixel {
  constexpr std::uint16_t Y_AT{ 0 };
  constexpr std::uint16_t X_AT{ 2 };
  ServiceLocals const     locals{ 4 };
  SetWord(ds, locals.At(X_AT), x);
  SetWord(ds, locals.At(Y_AT), y);
  opeu_view_pixel(ds, locals.At(X_AT), locals.At(Y_AT));
  return { Word(ds, locals.At(X_AT)), Word(ds, locals.At(Y_AT)) };
}
} // namespace opeu::game::detail::pixel_service

namespace opeu::game {
using detail::pixel_service::CellViewPixel;
} // namespace opeu::game
