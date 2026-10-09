/* Link: core/uiprim.c (0a04:000c..08ab image streams, portrait/face caches,
 * frames and palettes). The module takes a UiHost structure; its events,
 * from the host branch's comment (src/main/core/uiprim.c "Host model"):
 *   0 seek(handle,lo,hi,origin) -> DX:AX   FILEIO opeu_dos_seek
 *   1 read(handle,seg,off,count) -> AX     FILEIO opeu_dos_read_far
 *   2 blit(x,y,w,h,mode,seg,off)           VGALIB opeu_blit_far    0000:27ca
 *   3 draw(x,y,w,h,mode,seg,off)           VGALIB opeu_draw_far    0000:28b0
 *   4 draw_near(x,y,w,h,mode,ptr)          VGALIB opeu_draw_near   0000:28a8
 *   5 line(x1,y1,x2,y2,color)              VGALIB opeu_ui_line     0000:2d16
 *   6 box(x,y,w,h,color)                   VGALIB opeu_ui_box      0000:2ad2
 *   7 copy(a..f)                           VGALIB opeu_screen_copy 0000:301f
 *   8 palette(first,count,ptr)             VGALIB opeu_set_palette 0000:23d5
 *   9 decode(x,y,reader off,reader seg)    VGALIB opeu_picture_decode        0000:34a4
 *  10 decode_masked(x,y,reader off,seg)    VGALIB opeu_picture_decode_masked 0000:3490
 *  11 cursor(x,y)                          TEXTOUT opeu_context_position = 0480:0808
 *                                          (host opeu_army_position)
 * The decoder's reader is a far code pointer into this module's own segment:
 * 0a04:0132 opeu_stream_file_byte or 0a04:01fc opeu_stream_memory_byte
 * (FILE_READER / MEMORY_READER); both are recovered here and are called
 * through the same host instance. */
#include "link.h"

/* mirror of src/main/core/uiprim.c:197-198 (host branch) */
typedef uint32_t (*UiEvent)(uint8_t *ds, uint8_t *memory, uint16_t id, const uint16_t *args, uint16_t count);
typedef struct { uint8_t *memory; uint16_t ds_segment, tiles_segment, code_segment; UiEvent event; } UiHost;

#define UIPRIM_CODE_SEGMENT  0x0a04u   /* C0A04_UI */
#define UIPRIM_TILES_SEGMENT 0x3873u   /* _opeu_frame_tiles = 3873:0000 (symbols.json) */
#define UIPRIM_FILE_READER   0x0132u   /* 0a04:0132 opeu_stream_file_byte */
#define UIPRIM_MEMORY_READER 0x01fcu   /* 0a04:01fc opeu_stream_memory_byte */

static uint32_t uiprim_event(uint8_t *ds, uint8_t *memory, uint16_t id, const uint16_t *args, uint16_t count);

static UiHost uiprim_host = { 0, PORT_DS_SEGMENT, UIPRIM_TILES_SEGMENT, UIPRIM_CODE_SEGMENT, uiprim_event };

void *link_uiprim_host(void)
{
    uiprim_host.memory = port_arena;
    return &uiprim_host;
}

/* The two stream readers the decoder pulls bytes from. */
static uint8_t read_file_stream(void)
{
    return opeu_stream_file_byte(port_ds, link_uiprim_host());
}

static uint8_t read_memory_stream(void)
{
    return opeu_stream_memory_byte(port_ds, link_uiprim_host());
}

static VgaByteReader reader_of(uint16_t off, uint16_t seg)
{
    if (seg == UIPRIM_CODE_SEGMENT && off == UIPRIM_FILE_READER)
        return read_file_stream;
    if (seg == UIPRIM_CODE_SEGMENT && off == UIPRIM_MEMORY_READER)
        return read_memory_stream;
    port_fatal("uiprim: picture reader %04x:%04x is not a known stream reader", seg, off);
    return 0;
}

static uint32_t uiprim_event(uint8_t *ds, uint8_t *memory, uint16_t id, const uint16_t *args, uint16_t count)
{
    (void)ds;
    (void)memory;
    switch (id) {
    case 0:
        return (uint32_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3));
    case 1:
        return (uint16_t)opeu_dos_read_far((int16_t)ARG(0), ARG(1), ARG(2), ARG(3));
    case 2:
        opeu_blit_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        return 0;
    case 3:
        opeu_draw_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        return 0;
    case 4:
        opeu_draw_near(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), PORT_DS(ARG(5)));
        return 0;
    case 5:
        opeu_ui_line(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
        return 0;
    case 6:
        opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
        return 0;
    case 7:
        opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        return 0;
    case 8:
        opeu_set_palette((uint8_t)ARG(0), (uint8_t)ARG(1), PORT_DS(ARG(2)));
        return 0;
    case 9:
        opeu_picture_decode(ARG(0), ARG(1), reader_of(ARG(2), ARG(3)));
        return 0;
    case 10:
        opeu_picture_decode_masked(ARG(0), ARG(1), reader_of(ARG(2), ARG(3)));
        return 0;
    case 11:
        opeu_army_position(port_ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1));
        return 0;
    default:
        port_unhandled("uiprim", id, count);
    }
    return 0;
}
