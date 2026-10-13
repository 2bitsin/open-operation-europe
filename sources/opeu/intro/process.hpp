/* OPEN.EXE as a process in the port: the runner and its memory.
 *
 * Memory. OPEN is a separate DOS program: its own DGROUP (paragraph 093A of
 * its load image), its far data (the song bank, frame 0710) and its own DOS
 * memory blocks. While it runs, OpenMemory (open-memory.hpp) holds OPEN's
 * process in 640 KiB of conventional memory, plain data, where MAIN's
 * would be (the port's data arena, which the FM driver reads):
 *
 *   OPEN_LOAD_SEGMENT        load image (its code is native; nothing there)
 *   OPEN_BANK_SEGMENT:0002   the FM song bank (recovered/data/ofmusic.cpp)
 *   OPEN_DS_SEGMENT          DGROUP (the five recovered/data modules)
 *   OPEN_HEAP_SEGMENT ..     the DOS blocks OPEN allocates (443 KiB)
 *
 * The load segment is the port's choice (a DOS machine gives a different
 * one, depending on what is resident); OPEN only ever stores the segments
 * DOS returns and computes nothing from them. When OPEN has ended, the
 * arena is rebuilt with MAIN's image (port_memory_init), as DOS loaded
 * MAIN.EXE over the memory OPEN had freed.
 *
 * Stack. MS C's start-up runs OPEN with SS = DS and SP = 3040h (stack
 * segment 0AFE:1400 of the EXE header: 0AFE - 093A = 1C4h paragraphs). The
 * routines that keep stack buffers at DS addresses (the text engine's
 * conversion buffer, the variadic print
 * arguments) reserve them on OpenState's stack.
 */
#pragma once

#include "recovered.hpp"
#include "segment-bytes.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace opeu::intro {

inline constexpr Paragraph OPEN_LOAD_SEGMENT{ 0x0000 };
inline constexpr Paragraph OPEN_BANK_SEGMENT{ OPEN_LOAD_SEGMENT + BANK_FRAME };
inline constexpr Paragraph OPEN_DS_SEGMENT{ OPEN_LOAD_SEGMENT + 0x093a };
inline constexpr Paragraph OPEN_HEAP_SEGMENT{ OPEN_DS_SEGMENT + (OPEN_STACK_TOP >> 4) + 1 };

// A recovered data object and its place in OPEN's load image (image.cpp).
struct OpenImageObject {
  std::span<std::byte const> bytes;
  Paragraph                  seg; // 0 = DGROUP, else relative to the load segment
  NearWord                   off;
};
[[nodiscard]] auto OpenImageObjects() -> std::span<OpenImageObject const>;

}
