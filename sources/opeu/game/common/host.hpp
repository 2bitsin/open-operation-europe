// The host structures some modules take instead of a bare service, and the register block of the mouse and sound
// driver calls.
#pragma once

#include "common/events.hpp"

#include <cstdint>

// Driver registers as words or as byte halves.
struct WordRegs {
  std::uint16_t ax, bx, cx, dx, si, di, cflag;
};

struct ByteRegs {
  std::uint8_t al, ah, bl, bh, cl, ch, dl, dh;
};

union Regs {
  WordRegs x;
  ByteRegs h;
};

// The driver call of the mouse and sound services (int86): the seven register words go in and come back updated.
inline constexpr std::uint16_t INT86_CALL{ 1 };

inline auto Int86(Game& game, HostService service, Regs& r) -> std::uint16_t {
  std::uint16_t words[7]{ r.x.ax, r.x.bx, r.x.cx, r.x.dx, r.x.si, r.x.di, r.x.cflag };
  auto const    result{ service(game, INT86_CALL, words) };
  r.x = { words[0], words[1], words[2], words[3], words[4], words[5], words[6] };
  return result;
}

// The dialogs' service, load segment and the DS area of their stack buffers.
struct DialogHost {
  HostEvent     event;
  std::uint16_t load;
  std::uint16_t scratch;
};

// The text output's service.
struct TextHost {
  HostEvent event;
};

// The UI primitives' memory, segment numbers and service.
struct UiHost {
  std::uint8_t* memory;
  std::uint16_t ds_segment, tiles_segment, code_segment;
  UiEvent       event;
};
