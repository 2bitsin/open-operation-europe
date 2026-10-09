/* Struct hosts of the four recovered modules whose host entries take a host
 * structure instead of a bare service callback (moved from the former
 * port/link/core/textout.c, uiprim.c, dialogs.c and io/numdlg.c). Each
 * getter returns the single instance; its `event` is the module's service
 * table (svc_textout, svc_uiprim, svc_dialogs: game/, native build), the
 * other members are platform state the host branches read: the modeled
 * stack pointer, the arena and the original segment numbers. */
#include "glue.h"

#include "core/dialogs.h"
#include "core/textout.h"
#include "core/uiprim.h"
#include "io/numdlg.h"

/* ---- TEXTOUT: TextHost {event, sp} ------------------------------------------ *
 * textout keeps its automatic buffers below host->sp (ENTER/LEAVE). The
 * getter hands out the instance with sp = port_sp, the modeled SP of the
 * caller; while textout calls out through an event, svc_textout lowers
 * port_sp to textout's current sp (see its prologue) so that nested callers
 * allocate below textout's live buffers. */
static TextHost textout_host = { svc_textout, PORT_STACK_TOP };

void *port_textout_host(void)
{
    textout_host.sp = port_sp;
    return &textout_host;
}

/* svc_textout saves and restores the host's sp around an event. */
uint16_t *port_textout_host_sp(void)
{
    return &textout_host.sp;
}

/* ---- UIPRIM: UiHost {memory, ds_segment, tiles_segment, code_segment, event} - */
#define UIPRIM_CODE_SEGMENT  0x0a04u   /* C0A04_UI */
#define UIPRIM_TILES_SEGMENT 0x3873u   /* _opeu_frame_tiles = 3873:0000 (symbols.json) */
#define UIPRIM_FILE_READER   0x0132u   /* 0a04:0132 opeu_stream_file_byte */
#define UIPRIM_MEMORY_READER 0x01fcu   /* 0a04:01fc opeu_stream_memory_byte */

static UiHost uiprim_host = { 0, PORT_DS_SEGMENT, UIPRIM_TILES_SEGMENT, UIPRIM_CODE_SEGMENT, svc_uiprim };

void *port_uiprim_host(void)
{
    uiprim_host.memory = port_arena;
    return &uiprim_host;
}

/* The decoder's reader is a far code pointer into UIPRIM's own segment:
 * 0a04:0132 opeu_stream_file_byte or 0a04:01fc opeu_stream_memory_byte;
 * both are recovered and called through the same host instance. */
static uint8_t read_file_stream(void)
{
    return opeu_stream_file_byte(port_ds, port_uiprim_host());
}

static uint8_t read_memory_stream(void)
{
    return opeu_stream_memory_byte(port_ds, port_uiprim_host());
}

PortByteReader port_picture_reader(uint16_t off, uint16_t seg)
{
    if (seg == UIPRIM_CODE_SEGMENT && off == UIPRIM_FILE_READER)
        return read_file_stream;
    if (seg == UIPRIM_CODE_SEGMENT && off == UIPRIM_MEMORY_READER)
        return read_memory_stream;
    port_fatal("uiprim: picture reader %04x:%04x is not a known stream reader", seg, off);
    return 0;
}

/* ---- DIALOGS: DialogHost {event, load, scratch} ------------------------------ *
 * The original's near-stack buffers (at most 132 bytes, one owner active at
 * a time) live in the dialog routine's frame. The host takes one fixed DS
 * area for them: the bottom of the original 5120-byte stack (_end = ebe0h),
 * which the modeled stack (port.h, top ffe0h) never reaches in practice.
 * Segments are the original paragraph numbers (load segment 0). */
static const DialogHost dialogs_host = { svc_dialogs, 0u, PORT_DS_END };

void *port_dialogs_host(void)
{
    return (void *)&dialogs_host;
}

/* ---- NUMDLG ------------------------------------------------------------------ *
 * numdlg's routines take a bare service; the module builds its UiHost for
 * opeu_ui_outline internally (OutlineBridge, io/numdlg.c). The getter
 * returns that bridge shape ({ service }) for callers that want the
 * service through a host pointer. */
typedef uint16_t (*InputService)(uint8_t *ds, uint16_t call, uint16_t *words);
typedef struct { InputService service; } OutlineBridge;
static const OutlineBridge numdlg_bridge = { svc_numdlg };

void *port_numdlg_host(void)
{
    return (void *)&numdlg_bridge;
}
