#pragma once
// The pen of the game's one-bit drawing (glyphs, markers, highlights): an operation and a colour number, kept between
// calls, since a call naming no operation the pen knows draws with the previous pen. A 1 bit draws with it; a 0 bit
// leaves the pixel, except under AND, which clears the colour's bits there. What the pen makes of the number under a
// bit is a colour lookup (number-map.hpp).

#include "opeu/render/number-map.hpp"

#include <cstdint>

namespace opeu::render::detail::pen {

enum class PenOperation : std::uint8_t {
  NONE, // no pen chosen yet: nothing changes
  SET,
  XOR,
  AND,
  OR,
};

class BitPen {
public:
  // Operation code 0 set, 1 XOR, 2 AND, 3 OR, with `colour`; any other code keeps the pen and its colour.
  auto Select(std::uint8_t code, std::uint8_t colour) -> void;

  // What becomes of the number under a 1 bit, and under a 0 bit.
  [[nodiscard]] auto UnderSetBit() const noexcept -> NumberMap;
  [[nodiscard]] auto UnderClearBit() const noexcept -> NumberMap;
  [[nodiscard]] auto Operation() const noexcept -> PenOperation {
    return _operation;
  }
  [[nodiscard]] auto Colour() const noexcept -> std::uint8_t {
    return static_cast<std::uint8_t>(_colour & 0x0F);
  }

private:
  PenOperation _operation{ PenOperation::NONE };
  std::uint8_t _colour{ 7 };
};

}

namespace opeu::render {
using detail::pen::BitPen;
using detail::pen::PenOperation;
}
