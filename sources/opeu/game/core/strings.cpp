// Strings in the data segment, addressed by near pointer (offsets wrap at 16 bits), and ASCII case conversion.
#include "core/strings.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

// Copies source with its terminator to destination one byte at a time (an overlap behaves accordingly); returns
// destination.
uint16_t opeu_string_copy(uint8_t ds[65536], uint16_t destination, uint16_t source) {
  uint16_t const result{ destination };
  while ((ds[destination++] = ds[source++]) != 0) {
  }
  return result;
}

// Appends source to the string at destination; returns destination.
uint16_t opeu_string_append(uint8_t ds[65536], uint16_t destination, uint16_t source) {
  uint16_t const result{ destination };
  while (ds[destination]) ++destination;
  while ((ds[destination++] = ds[source++]) != 0) {
  }
  return result;
}

uint16_t opeu_string_length(uint8_t ds[65536], uint16_t string) {
  uint16_t const start{ string };
  while (ds[string]) ++string;
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

auto port_routine_opeu_string_copy(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_string_copy(ds, Arg(args, count, 1), Arg(args, count, 0));
}
