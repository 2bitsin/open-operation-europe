/* OPEN.EXE's process memory in the data arena: DGROUP and far-data image,
 * the DOS memory blocks and the modeled stack. See process.hpp. */
#include "port.hpp"
#include "process.hpp"

#include <stdlib.h>
#include <string.h>

namespace opeu::intro {
namespace {
constexpr int MAX_BLOCKS{ 16 };
}

uint8_t* open_ds;
uint16_t open_sp = OPEN_STACK_TOP;

/* DOS memory blocks of the OPEN process: [segment, paragraphs), first fit
 * from OPEN_HEAP_SEGMENT, one arena paragraph (the MCB) before each. */
static struct {
  uint16_t seg, paras;
} blocks[MAX_BLOCKS];
static int block_count;

void open_memory_init(void) {
  unsigned i;
  if (!port_arena) {
    port_arena = (uint8_t*)calloc(1, PORT_ARENA_SIZE + 0x10000u);
    if (!port_arena) port_fatal("out of memory for the far-data arena");
  } else {
    memset(port_arena, 0, PORT_ARENA_SIZE);
  }
  open_ds = port_arena + (uint32_t)OPEN_DS_SEGMENT * 16u;
  for (i = 0; i < open_image_object_count; i++) {
    const OpenImageObject* o = &open_image_objects[i];
    uint32_t seg = o->seg ? OPEN_LOAD_SEGMENT + o->seg : OPEN_DS_SEGMENT;
    uint32_t linear = seg * 16u + o->off;
    if (linear + o->size > PORT_ARENA_SIZE) port_fatal("OPEN data object %s outside the arena", o->name);
    memcpy(port_arena + linear, o->object, o->size);
  }
  block_count = 0;
  open_sp = OPEN_STACK_TOP;
}

uint16_t open_dos_alloc(uint16_t paragraphs) {
  uint32_t seg = OPEN_HEAP_SEGMENT;
  int i, j;
  for (;;) {
    int clash = 0;
    for (i = 0; i < block_count; i++) {
      uint32_t s = blocks[i].seg - 1u, e = (uint32_t)blocks[i].seg + blocks[i].paras;
      if (seg - 1u < e && seg + paragraphs > s) {
        seg = e + 1u;
        clash = 1;
      }
    }
    if (!clash) break;
  }
  if (seg + paragraphs > PORT_ARENA_SIZE / 16u || block_count == MAX_BLOCKS) {
    port_trace("mem", "OPEN dos_alloc %04x paragraphs: no memory", paragraphs);
    return 0;
  }
  for (i = 0; i < block_count && blocks[i].seg < seg; i++);
  for (j = block_count; j > i; j--) blocks[j] = blocks[j - 1];
  blocks[i].seg = (uint16_t)seg;
  blocks[i].paras = paragraphs;
  block_count++;
  port_trace("mem", "OPEN dos_alloc %04x paragraphs -> %04x", paragraphs, (unsigned)seg);
  return (uint16_t)seg;
}

int open_dos_free(uint16_t segment) {
  int i;
  for (i = 0; i < block_count; i++)
    if (blocks[i].seg == segment) {
      for (; i < block_count - 1; i++) blocks[i] = blocks[i + 1];
      block_count--;
      return 0;
    }
  return 9;   /* DOS: invalid memory block address */
}

uint16_t open_stack_enter(uint16_t bytes) {
  uint16_t sp = (uint16_t)(open_sp - 4);
  if ((uint32_t)open_sp < (uint32_t)OPEN_DS_END + 4u + bytes)
    port_fatal("OPEN: modeled stack overflow (sp=%04x, %u bytes)", open_sp, bytes);
  open_sp = (uint16_t)(open_sp - 4 - bytes);
  return sp;
}

void open_stack_leave(uint16_t bytes) {
  open_sp = (uint16_t)(open_sp + 4 + bytes);
}

}
