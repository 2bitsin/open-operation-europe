#pragma once
// The pen of the game's one-bit drawing (glyphs, markers, highlights): an operation and a colour number, kept between
// calls, since a call naming no operation the pen knows draws with the previous pen. A 1 bit draws with it; a 0 bit
// leaves the pixel, except under AND, which clears the colour's bits there.

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

  // The colour number under a 1 bit after drawing it, and under a 0 bit.
  [[nodiscard]] auto UnderSetBit(std::uint8_t number) const noexcept -> std::uint8_t;
  [[nodiscard]] auto UnderClearBit(std::uint8_t number) const noexcept -> std::uint8_t;
  // Whether it draws by setting its colour (a shape blit), not by combining with the screen.
  [[nodiscard]] auto Sets() const noexcept -> bool {
    return _operation == PenOperation::SET;
  }
  [[nodiscard]] auto Draws() const noexcept -> bool {
    return _operation != PenOperation::NONE;
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
