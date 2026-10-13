// The opening's far data and the screen operations on it (segment-buffers.hpp).
#include "segment-buffers.hpp"

#include "open-services.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

namespace opeu::intro::detail::segment_buffers {
namespace {
auto Word(int value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value);
}

// The blitters read their source with LODS through DS:SI, which wraps inside the segment: the bytes in place, or a
// wrapped copy when the image would run past offset FFFFh (OPEN's own images never do).
template <class _Blit>
auto WithSource(Open& open, FarAddress at, std::size_t size, _Blit blit) -> void {
  auto const segment{ open.Memory().Segment(at.segment) };
  if (at.offset + size <= SEGMENT_SIZE) {
    blit(segment.subspan(at.offset, size).data());
    return;
  }
  auto const wrapped{ std::views::iota(0uz, size) | std::views::transform([&](std::size_t step) { return segment[static_cast<std::uint16_t>(at.offset + step)]; }) | std::ranges::to<std::vector>() };
  blit(wrapped.data());
}

// The area in the services' words.
auto ColumnArea(ScreenArea area) -> services::Area {
  return { .x = Word(area.x), .y = Word(area.y), .width = Word(area.width), .height = Word(area.height) };
}

// A saved rectangle of far memory as the compositing services take it.
auto Saved(Open& open, FarRectangle rectangle) -> services::SavedRectangle {
  return {
    .bytes = { open.Memory().Segment(rectangle.segment), rectangle.offset },
      .width = Word(rectangle.width), .height = Word(rectangle.height)
  };
}

// Source bytes of an image: `planes` bytes per column and line (row doubling reads each row once).
auto ImageBytes(ScreenArea area, std::size_t planes) -> std::size_t {
  return std::size_t{ Word(area.width) } * Word(area.height) * planes;
}
} // namespace

auto BlitFar(Open& open, ScreenArea area, FarImage image) -> void {
  WithSource(open, { image.segment, image.offset }, ImageBytes(area, 4),
             [&](auto const* bytes) { opeu_bitmap_restore(Word(area.x), Word(area.y), Word(area.width), Word(area.height), image.mode, bytes); });
}

auto DrawFar(Open& open, ScreenArea area, FarImage image) -> void {
  WithSource(open, { image.segment, image.offset }, ImageBytes(area, 1), [&](auto const* bytes) { opeu_draw_far(Word(area.x), Word(area.y), Word(area.width), Word(area.height), image.mode, bytes); });
}

auto SaveScreen(Open& open, ScreenArea area, FarAddress destination) -> void {
  services::SaveScreenRectangle(ColumnArea(area), { open.Memory().Segment(destination.segment), destination.offset });
}

auto CopyFar(Open& open, FarAddress from, FarBuffer to) -> void {
  services::CopyFarWords({ open.Memory().Segment(from.segment), from.offset }, { open.Memory().Segment(to.segment), to.offset }, to.size);
}

auto OrSprite(Open& open, FarRectangle target, FarAddress sprite) -> void {
  services::OrIntoBuffer(Saved(open, target), { open.Memory().Segment(sprite.segment), sprite.offset });
}

auto AndMask(Open& open, FarRectangle target, FarAddress mask) -> void {
  services::AndWithMask(Saved(open, target), { open.Memory().Segment(mask.segment), mask.offset });
}

auto Dissolve(Open& open, ScreenPoint at, DissolveSource source) -> void {
  services::DissolveStep({ .x = Word(at.x), .y = Word(at.y) }, { open.Memory().Segment(source.segment), source.offset }, source.pattern);
}

auto ReadFar(Open& open, std::int16_t handle, FarBuffer buffer) -> std::int16_t {
  return opeu_dos_read(handle, open.Memory().Bytes(buffer.segment, buffer.offset, buffer.size).data(), buffer.size);
}

auto FarByte(Open& open, FarAddress at) -> std::uint8_t {
  return open.Memory().Segment(at.segment)[at.offset];
}
} // namespace opeu::intro::detail::segment_buffers
