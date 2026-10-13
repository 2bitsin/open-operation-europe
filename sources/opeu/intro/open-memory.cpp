// OPEN.EXE's memory (open-memory.hpp).
#include "open-memory.hpp"

#include "port.hpp"
#include "process.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::intro::detail::open_memory {
namespace {
constexpr std::size_t PARAGRAPH{ 16 };
constexpr std::size_t MAX_BLOCKS{ 16 };

// The paragraph an image object lies in: DGROUP's, or one relative to the load segment.
auto ObjectParagraph(OpenImageObject const& object) -> std::uint32_t {
  return (object.segment != 0u) ? std::uint32_t{ OPEN_LOAD_SEGMENT } + object.segment : std::uint32_t{ OPEN_DS_SEGMENT };
}
} // namespace

OpenMemory::OpenMemory(std::span<std::uint8_t> bytes) : _bytes{ bytes } {
  Expects(bytes.size() > SEGMENT_SIZE, "the memory holds a segment past its end");
}

auto OpenMemory::Load() -> void {
  std::ranges::fill(Conventional(), std::uint8_t{ 0 });
  std::ranges::for_each(OpenImageObjects(), [&](OpenImageObject const& object) {
    auto const linear{ (ObjectParagraph(object) * PARAGRAPH) + object.offset };
    Expects(linear + object.bytes.size() <= Conventional().size(), "an OPEN data object lies in conventional memory");
    std::ranges::copy(object.bytes, std::as_writable_bytes(_bytes.subspan(linear)).begin());
  });
  _blocks.clear();
}

// Each block clashing with the candidate moves it past the block's end and its MCB; the blocks are checked in address
// order, and again until a pass moves nothing.
auto OpenMemory::FirstFit(Paragraph paragraphs) const -> std::uint32_t {
  std::uint32_t candidate{ OPEN_HEAP_SEGMENT };
  bool          moved{ true };
  while (moved) {
    moved = false;
    std::ranges::for_each(_blocks, [&](Block const& block) {
      std::uint32_t const start{ block.segment - 1u };
      std::uint32_t const end{ std::uint32_t{ block.segment } + block.paragraphs };
      if (candidate - 1u < end && candidate + paragraphs > start) {
        candidate = end + 1u;
        moved     = true;
      }
    });
  }
  return candidate;
}

auto OpenMemory::Allocate(Paragraph paragraphs) -> Paragraph {
  std::uint32_t const segment{ FirstFit(paragraphs) };
  if (segment + paragraphs > Conventional().size() / PARAGRAPH || _blocks.size() == MAX_BLOCKS) {
    port_trace("mem", "OPEN dos_alloc %04x paragraphs: no memory", paragraphs);
    return 0;
  }
  auto const place{ std::ranges::find_if(_blocks, [&](Block const& block) { return block.segment >= segment; }) };
  _blocks.insert(place, Block{ .segment = static_cast<Paragraph>(segment), .paragraphs = paragraphs });
  port_trace("mem", "OPEN dos_alloc %04x paragraphs -> %04x", paragraphs, static_cast<unsigned>(segment));
  return static_cast<Paragraph>(segment);
}

auto OpenMemory::Free(Paragraph segment) -> bool {
  return std::erase_if(_blocks, [&](Block const& block) { return block.segment == segment; }) != 0;
}

auto OpenMemory::Segment(Paragraph paragraph) const -> intro::Segment {
  return _bytes.subspan(std::size_t{ paragraph } * PARAGRAPH).first<SEGMENT_SIZE>();
}

auto OpenMemory::Bytes(Paragraph paragraph, NearWord offset, std::size_t size) const -> std::span<std::uint8_t> {
  Expects((std::size_t{ paragraph } * PARAGRAPH) + offset + size <= _bytes.size(), "a far transfer stays in OPEN's memory");
  return _bytes.subspan((std::size_t{ paragraph } * PARAGRAPH) + offset, size);
}

auto OpenMemory::Conventional() const -> std::span<std::uint8_t> {
  return _bytes.first(_bytes.size() - SEGMENT_SIZE);
}
} // namespace opeu::intro::detail::open_memory
