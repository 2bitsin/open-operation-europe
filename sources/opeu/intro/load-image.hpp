// OPEN.EXE as a process in the port: its load image and the memory map around it.
//
// Memory. OPEN is a separate DOS program: its own DGROUP, its far data (the
// song bank) and its own DOS memory blocks. While it runs, OpenMemory (open-memory.hpp) holds OPEN's
// process in 640 KiB of conventional memory, plain data, where MAIN's
// would be (the port's data arena, which the FM driver reads):
//
//   OPEN_LOAD_SEGMENT        load image (its code is native; nothing there)
//   OPEN_BANK_SEGMENT:0002   the FM song bank (recovered/data/song-bank.cpp)
//   OPEN_DS_SEGMENT          DGROUP (the five recovered/data modules)
//   OPEN_HEAP_SEGMENT ..     the DOS blocks OPEN allocates (443 KiB)
//
// The load segment is the port's choice (a DOS machine gives a different
// one, depending on what is resident); OPEN only ever stores the segments
// DOS returns and computes nothing from them. When OPEN has ended, the
// arena is rebuilt with MAIN's image (port_memory_init), as DOS loaded
// MAIN.EXE over the memory OPEN had freed.
//
// Stack. The C start-up runs OPEN with SS = DS and SP = OPEN_STACK_TOP. The
// routines that keep stack buffers at DS addresses (the text console's
// conversion buffer, the print's arguments) reserve them on OpenState's
// stack.
#pragma once

#include "open-state.hpp"
#include "segment-bytes.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace opeu::intro::detail::load_image {

// The song bank's frame in the load image and its offset in that frame: the address the FM driver is opened with.
inline constexpr Paragraph BANK_FRAME{ 0x0710 };
inline constexpr NearWord  BANK_OFFSET{ 0x0002 };

inline constexpr Paragraph OPEN_LOAD_SEGMENT{ 0x0000 };
inline constexpr Paragraph OPEN_BANK_SEGMENT{ OPEN_LOAD_SEGMENT + BANK_FRAME };
inline constexpr Paragraph OPEN_DS_SEGMENT{ OPEN_LOAD_SEGMENT + 0x093a };
inline constexpr Paragraph OPEN_HEAP_SEGMENT{ OPEN_DS_SEGMENT + (OPEN_STACK_TOP >> 4) + 1 };

// A recovered data object and its place in OPEN's load image.
struct OpenImageObject {
  std::span<std::byte const> bytes;
  Paragraph                  segment; // 0 = DGROUP, else relative to the load segment
  NearWord                   offset;
};
inline constexpr std::size_t OPEN_IMAGE_OBJECT_COUNT{ 71 };
// Every recovered data object and its place.
[[nodiscard]] auto OpenImageObjects() -> std::array<OpenImageObject, OPEN_IMAGE_OBJECT_COUNT>;
} // namespace opeu::intro::detail::load_image

namespace opeu::intro {
using detail::load_image::BANK_FRAME;
using detail::load_image::BANK_OFFSET;
using detail::load_image::OPEN_BANK_SEGMENT;
using detail::load_image::OPEN_DS_SEGMENT;
using detail::load_image::OPEN_HEAP_SEGMENT;
using detail::load_image::OPEN_LOAD_SEGMENT;
using detail::load_image::OpenImageObject;
using detail::load_image::OPEN_IMAGE_OBJECT_COUNT;
using detail::load_image::OpenImageObjects;
} // namespace opeu::intro
