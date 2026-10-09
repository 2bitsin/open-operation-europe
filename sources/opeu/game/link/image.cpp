/* The initial data image: MAIN's data objects and module data copied into
 * the arena the glue has reset (glue/core/memory.cpp). */
#include "port.hpp"

#include <string.h>

void port_memory_init(void) {
  unsigned i;
  port_memory_reset();
  for (i = 0; i < port_image_object_count; i++) {
    const PortImageObject* o      = &port_image_objects[i];
    uint32_t               linear = (uint32_t)o->seg * 16u + o->off;
    if (linear + o->size > PORT_ARENA_SIZE)
      port_fatal("data object %s outside the arena", o->name);
    memcpy(port_arena + linear, o->object, o->size);
  }
  for (i = 0; i < port_module_data_count; i++) {
    const PortImageBytes* b      = &port_module_data[i];
    uint32_t              linear = (uint32_t)b->seg * 16u + b->off;
    if (linear + b->size > PORT_ARENA_SIZE)
      port_fatal("module data of %s outside the arena", b->module);
    memcpy(port_arena + linear, b->bytes, b->size);
  }
}
