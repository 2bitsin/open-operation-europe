#include "opeu/render/pen.hpp"

#include <array>

namespace opeu::render::detail::pen {

auto BitPen::Select(std::uint8_t const code, std::uint8_t const colour) -> void {
  static constexpr auto OPERATIONS{
    std::array{ PenOperation::SET, PenOperation::XOR, PenOperation::AND, PenOperation::OR }
  };
  if (code >= OPERATIONS.size()) return;
  _operation = OPERATIONS.at(code);
  _colour    = colour;
}

auto BitPen::UnderSetBit(std::uint8_t const number) const noexcept -> std::uint8_t {
  switch (_operation) {
  case PenOperation::SET:  return Colour();
  case PenOperation::XOR:  return static_cast<std::uint8_t>(number ^ Colour());
  case PenOperation::OR:   return static_cast<std::uint8_t>(number | Colour());
  case PenOperation::AND:
  case PenOperation::NONE: break;
  }
  return number;
}

auto BitPen::UnderClearBit(std::uint8_t const number) const noexcept -> std::uint8_t {
  if (_operation == PenOperation::AND) return static_cast<std::uint8_t>(number & ~Colour());
  return number;
}

}
