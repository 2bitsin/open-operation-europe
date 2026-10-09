/* Far-data arena, DGROUP image, DOS memory blocks and the stack model. */
#include "port.h"

#include <stdlib.h>
#include <string.h>

uint8_t *port_arena;
uint8_t *port_ds;
uint16_t port_sp = PORT_STACK_TOP;

/* DOS memory blocks: a first-fit list of [segment, paragraphs). */
#define MAX_BLOCKS 64
static struct { uint16_t seg, paras; } blocks[MAX_BLOCKS];
static int block_count;

void port_memory_init(void)
{
    unsigned i;
    if (!port_arena) {
        port_arena = (uint8_t *)calloc(1, PORT_ARENA_SIZE + 0x10000u);
        if (!port_arena)
            port_fatal("out of memory for the far-data arena");
    } else {
        memset(port_arena, 0, PORT_ARENA_SIZE);
    }
    port_ds = port_arena + (uint32_t)PORT_DS_SEGMENT * 16u;
    for (i = 0; i < port_image_object_count; i++) {
        const PortImageObject *o = &port_image_objects[i];
        uint32_t linear = (uint32_t)o->seg * 16u + o->off;
        if (linear + o->size > PORT_ARENA_SIZE)
            port_fatal("data object %s outside the arena", o->name);
        memcpy(port_arena + linear, o->object, o->size);
    }
    for (i = 0; i < port_module_data_count; i++) {
        const PortImageBytes *b = &port_module_data[i];
        uint32_t linear = (uint32_t)b->seg * 16u + b->off;
        if (linear + b->size > PORT_ARENA_SIZE)
            port_fatal("module data of %s outside the arena", b->module);
        memcpy(port_arena + linear, b->bytes, b->size);
    }
    block_count = 0;
    port_sp = PORT_STACK_TOP;
}

uint8_t *port_far(uint16_t seg, uint16_t off)
{
    uint32_t linear = (uint32_t)seg * 16u + off;
    if (linear >= PORT_ARENA_SIZE + 0x10000u)
        port_fatal("far access %04x:%04x outside the arena", seg, off);
    return port_arena + linear;
}

uint16_t port_ds_word(uint16_t off)
{
    return (uint16_t)(port_ds[off] | port_ds[(uint16_t)(off + 1)] << 8);
}

void port_ds_set_word(uint16_t off, uint16_t value)
{
    port_ds[off] = (uint8_t)value;
    port_ds[(uint16_t)(off + 1)] = (uint8_t)(value >> 8);
}

uint16_t port_segment_of(const void *p)
{
    const uint8_t *b = (const uint8_t *)p;
    if (b < port_arena || b >= port_arena + PORT_ARENA_SIZE)
        port_fatal("pointer %p is not in the far-data arena", p);
    return (uint16_t)((size_t)(b - port_arena) >> 4);
}

uint16_t port_offset_of(const void *p, uint16_t seg)
{
    const uint8_t *b = (const uint8_t *)p;
    size_t linear = (size_t)(b - port_arena);
    if (b < port_arena || linear < (size_t)seg * 16u || linear - (size_t)seg * 16u > 0xffffu)
        port_fatal("pointer %p is not addressable from segment %04x", p, seg);
    return (uint16_t)(linear - (size_t)seg * 16u);
}

uint16_t port_dos_alloc(uint16_t paragraphs)
{
    uint32_t seg = PORT_HEAP_SEGMENT;
    int i, j;
    for (;;) {
        int clash = 0;
        for (i = 0; i < block_count; i++) {
            uint32_t s = blocks[i].seg, e = s + blocks[i].paras;
            if (seg < e && seg + paragraphs > s) {
                seg = e;
                clash = 1;
            }
        }
        if (!clash)
            break;
    }
    if (seg + paragraphs > PORT_ARENA_SIZE / 16u || block_count == MAX_BLOCKS)
        return 0;
    for (i = 0; i < block_count && blocks[i].seg < seg; i++)
        ;
    for (j = block_count; j > i; j--)
        blocks[j] = blocks[j - 1];
    blocks[i].seg = (uint16_t)seg;
    blocks[i].paras = paragraphs;
    block_count++;
    memset(port_arena + seg * 16u, 0, (size_t)paragraphs * 16u);
    port_trace("mem", "dos_alloc %04x paragraphs -> %04x", paragraphs, (unsigned)seg);
    return (uint16_t)seg;
}

int port_dos_free(uint16_t segment)
{
    int i;
    for (i = 0; i < block_count; i++)
        if (blocks[i].seg == segment) {
            for (; i < block_count - 1; i++)
                blocks[i] = blocks[i + 1];
            block_count--;
            return 0;
        }
    return 9;   /* DOS: invalid memory block address */
}

/* The callee's entry SP points at a 4-byte far return address slot just
 * below the caller's frame; `bytes` more are reserved below it for the
 * callee's locals and the frames of the routines it calls internally. */
uint16_t port_stack_enter(uint16_t bytes)
{
    uint16_t sp = (uint16_t)(port_sp - 4);
    if ((uint32_t)port_sp < (uint32_t)PORT_DS_END + 4u + bytes)
        port_fatal("modeled DGROUP stack overflow (sp=%04x, %u bytes)", port_sp, bytes);
    port_sp = (uint16_t)(port_sp - 4 - bytes);
    return sp;
}

void port_stack_leave(uint16_t bytes)
{
    port_sp = (uint16_t)(port_sp + 4 + bytes);
}
