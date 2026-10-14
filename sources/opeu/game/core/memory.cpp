// Byte blocks of the data segment copied and filled, and far strings copied into a text.
//
// Near offsets wrap at 16 bits. The block routines work one byte at a time, so overlapping ranges see the effects of the
// bytes already moved.
#include "core/memory.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "common/text.hpp"

#include <cstdint>
#include <ranges>
#include <span>
#include <string>

// Copies size bytes from source to destination: forwards when the destination lies below the source, otherwise
// backwards from the end.
void opeu_copy_bytes(Game& game, uint16_t source, uint16_t destination, uint16_t size) {
  if (destination < source) {
    for (; size != 0; size--) Byte(game, destination++) = Byte(game, source++);
  } else {
    destination += size;
    source      += size;
    while (size) {
      Byte(game, --destination) = Byte(game, --source);
      size--;
    }
  }
}

// Copies the far string, from the start of `far_text` to its terminator, into the destination; returns the
// destination.
TextTarget opeu_string_copy_far(Game& game, TextTarget destination, std::span<std::uint8_t const> far_text) {
  auto const text{ far_text | std::views::take_while([](std::uint8_t byte) { return byte != 0; }) | std::views::transform([](std::uint8_t byte) { return static_cast<char>(byte); })
                   | std::ranges::to<std::string>() };
  StoreText(Data(game), destination, text);
  return destination;
}

void opeu_fill_bytes(Game& game, uint16_t buffer, uint16_t size, uint8_t value) {
  for (; size; size--) Byte(game, buffer++) = value;
}
