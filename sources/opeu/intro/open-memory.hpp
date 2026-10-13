// OPEN.EXE's memory: its load image (the song bank and DGROUP placed from the recovered data modules), the DOS blocks
// it allocates first fit above its stack, and the 64 KiB view of any paragraph. The bytes are the conventional memory
// the resident FM driver also reads (the song bank, and past its table the shutdown's song 5), so the view lies where
// the driver looks: the data arena, until the driver reads from memory handed to it.
#pragma once

#include "segment-bytes.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace opeu::intro::detail::open_memory {

class OpenMemory {
public:
  // `bytes`: the conventional memory, then 64 KiB more so that every paragraph below its end has a whole segment.
  explicit OpenMemory(std::span<std::uint8_t> bytes);

  // OPEN as DOS loads it: memory cleared, the data objects at their places, no block allocated.
  auto Load() -> void;
  // INT 21h AH=48h: the first block of `paragraphs` that fits above OPEN's stack, one paragraph (the MCB) before it;
  // 0 when none does.
  auto Allocate(Paragraph paragraphs) -> Paragraph;
  // INT 21h AH=49h; whether the segment was an allocated block.
  auto Free(Paragraph segment) -> bool;
  // The 64 KiB a segment register addresses.
  [[nodiscard]] auto Segment(Paragraph paragraph) const -> segment_bytes::Segment;
  // `size` bytes from a far address on, without wrapping (a DOS transfer).
  [[nodiscard]] auto Bytes(Paragraph paragraph, NearWord offset, std::size_t size) const -> std::span<std::uint8_t>;

private:
  struct Block {
    Paragraph segment;
    Paragraph paragraphs;
  };

  // The memory below the last segment's extension: where DOS loads and allocates.
  [[nodiscard]] auto Conventional() const -> std::span<std::uint8_t>;
  [[nodiscard]] auto FirstFit(Paragraph paragraphs) const -> std::uint32_t;

  std::span<std::uint8_t> _bytes;
  std::vector<Block>      _blocks;
};
} // namespace opeu::intro::detail::open_memory

namespace opeu::intro {
using detail::open_memory::OpenMemory;
} // namespace opeu::intro
