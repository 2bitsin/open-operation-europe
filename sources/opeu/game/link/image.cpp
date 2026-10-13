// The initial data image: MAIN's far data objects and module data copied, the data modules' fields of the data segment
// written, into the arena the glue has reset (glue/core/memory.cpp).
#include "data/image.hpp"
#include "common/data-segment.hpp"
#include "port.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {
// The arena's bytes at segment:offset that a block of `size` bytes occupies, or nothing when the block passes its end.
auto ArenaBlock(std::uint16_t segment, std::uint16_t offset, std::size_t size) -> std::span<std::uint8_t> {
  auto const linear{ std::size_t{ segment } * 16 + offset };
  if (linear + size > PORT_ARENA_SIZE) return {};
  return std::span{ port_arena, PORT_ARENA_SIZE }.subspan(linear, size);
}

// Copies a far data object of MAIN to its address.
auto PlaceObject(PortImageObject const& object) -> void {
  auto const target{ ArenaBlock(object.seg, object.off, object.size) };
  if (target.size() != object.size) port_fatal("data object %s outside the arena", object.name);
  std::ranges::copy(std::span{ static_cast<std::uint8_t const*>(object.object), object.size }, target.begin());
}

// Copies a code module's initialized data to its address.
auto PlaceModuleData(PortImageBytes const& block) -> void {
  auto const target{ ArenaBlock(block.seg, block.off, block.size) };
  if (target.size() != block.size) port_fatal("module data of %s outside the arena", block.module);
  std::ranges::copy(std::span{ block.bytes, block.size }, target.begin());
}
} // namespace

void port_memory_init() {
  port_memory_reset();
  std::ranges::for_each(std::span{ port_image_objects, port_image_object_count }, PlaceObject);
  std::ranges::for_each(std::span{ port_module_data, port_module_data_count }, PlaceModuleData);
  opeu::game::InitialDataSegment(opeu::game::Data(port_ds));
}
