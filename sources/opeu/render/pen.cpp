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

auto BitPen::UnderSetBit() const noexcept -> NumberMap {
  switch (_operation) {
  case PenOperation::SET:  return Solid(Colour());
  case PenOperation::XOR:  return Lookup(Operation::XOR, Colour());
  case PenOperation::OR:   return Lookup(Operation::OR, Colour());
  case PenOperation::AND:
  case PenOperation::NONE: break;
  }
  return SAME;
}

auto BitPen::UnderClearBit() const noexcept -> NumberMap {
  if (_operation == PenOperation::AND) return Lookup(Operation::AND, static_cast<std::uint8_t>(~Colour() & 0x0F));
  return SAME;
}

}
