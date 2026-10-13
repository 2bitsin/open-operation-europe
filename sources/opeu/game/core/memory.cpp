// Byte blocks: copy and fill in the data segment, and copies from far segments.
//
// Near offsets wrap at 16 bits; a far address advances only its offset, so far accesses wrap inside their segment.
// A far argument is a segment base plus a 16-bit offset, and the base may be the data segment itself. Every routine
// works one byte at a time, so overlapping ranges see the effects of the bytes already moved.
#include "core/memory.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

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

void opeu_copy_far_to_near(Game& game, uint8_t* source_segment, uint16_t source, uint16_t destination, uint16_t size) {
  for (; size; size--) Byte(game, destination++) = source_segment[source++];
}

// The same transfer as opeu_copy_far_to_near, under the name its other callers use.
void opeu_read_far_bytes(Game& game, uint8_t* source_segment, uint16_t source, uint16_t destination, uint16_t size) {
  opeu_copy_far_to_near(game, source_segment, source, destination, size);
}

// Copies the far string with its terminator to destination; returns destination.
uint16_t opeu_string_copy_far(Game& game, uint16_t destination, uint8_t* source_segment, uint16_t source) {
  uint16_t const result{ destination };
  while ((Byte(game, destination++) = source_segment[source++]) != 0) {
  }
  return result;
}

void opeu_fill_bytes(Game& game, uint16_t buffer, uint16_t size, uint8_t value) {
  for (; size; size--) Byte(game, buffer++) = value;
}
