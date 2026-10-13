// Strings in the data segment, addressed by near pointer (offsets wrap at 16 bits), and ASCII case conversion.
#include "core/strings.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

// Copies source with its terminator to destination one byte at a time (an overlap behaves accordingly); returns
// destination.
uint16_t opeu_string_copy(Game& game, uint16_t destination, uint16_t source) {
  uint16_t const result{ destination };
  while ((Byte(game, destination++) = Byte(game, source++)) != 0) {
  }
  return result;
}

// Appends source to the string at destination; returns destination.
uint16_t opeu_string_append(Game& game, uint16_t destination, uint16_t source) {
  uint16_t const result{ destination };
  while (Byte(game, destination)) ++destination;
  while ((Byte(game, destination++) = Byte(game, source++)) != 0) {
  }
  return result;
}

uint16_t opeu_string_length(Game& game, uint16_t string) {
  uint16_t const start{ string };
  while (Byte(game, string)) ++string;
  return string - start;
}

uint8_t opeu_ascii_upper(uint8_t code) {
  if (code >= 'a' && code <= 'z') return code - 32;
  return code;
}

uint8_t opeu_ascii_lower(uint8_t code) {
  if (code >= 'A' && code <= 'Z') return code + 32;
  return code;
}
