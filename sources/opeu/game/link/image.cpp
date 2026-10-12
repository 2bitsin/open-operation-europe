// The initial data image: MAIN's far data objects and module data copied, the data modules' fields of the data segment
// written, into the arena the glue has reset (glue/core/memory.cpp).
#include "port.hpp"
#include "common/data-segment.hpp"
#include "data/image.hpp"

#include <cstring>
#include <span>

void port_memory_init() {
  port_memory_reset();
  for (auto const& object :
       std::span{ port_image_objects, port_image_object_count }) {
    auto const linear{ static_cast<std::uint32_t>(object.seg) * 16u +
                       object.off };
    if (linear + object.size > PORT_ARENA_SIZE)
      port_fatal("data object %s outside the arena", object.name);
    std::memcpy(port_arena + linear, object.object, object.size);
  }
  for (auto const& block :
       std::span{ port_module_data, port_module_data_count }) {
    auto const linear{ static_cast<std::uint32_t>(block.seg) * 16u +
                       block.off };
    if (linear + block.size > PORT_ARENA_SIZE)
      port_fatal("module data of %s outside the arena", block.module);
    std::memcpy(port_arena + linear, block.bytes, block.size);
  }
  opeu::game::InitialDataSegment(opeu::game::Data(port_ds));
}
