/* Operation Europe native port: the shared contract of the port layer.
 *
 * The game logic is the recovered C of src/main/{core,data,io,map,units},
 * compiled from its host branches (the `#else` side of
 * `#if defined(_MSC_VER) && _MSC_VER < 1000`). Those branches keep the
 * original 16-bit data layout: every near pointer is a DS offset into a
 * 64 KiB `ds[]` image, far pointers are segment:offset pairs, and the calls
 * that leave a module go through a service callback ("event") with numbered
 * services or, in the battle modules, with the routine's original address
 * as its identifier.
 *
 * The port compiles the generated game/ tree (tools/make_game_tree.py),
 * where each module's services are direct calls in its native-build section
 * (tools/game-services.json). This header provides what those sections and
 * the platform glue (port/include/glue.h) need to reach the recovered
 * routines and the hand-written native services (port/services):
 *
 *  - the data layout: one far-data arena holding the original FAR_DATA
 *    segments, DGROUP (`port_ds`) and the DOS memory blocks the game
 *    allocates, addressed by segment:offset exactly as the host branches
 *    compute it. It is plain data; nothing executes from it.
 *  - a model of the DGROUP stack pointer for the host branches that take
 *    `sp` and keep their stack buffers at original DS addresses;
 *  - the routine link table: identifier (original routine address) ->
 *    native C function, for the address-keyed service callbacks;
 *  - diagnostics.
 */
#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- data layout ------------------------------------------------------- */

#define PORT_DS_SEGMENT   0x4712u     /* DGROUP paragraph in the original layout */
#define PORT_DS_END       0xebe0u     /* _end: first byte past DGROUP data+BSS */
#define PORT_STACK_TOP    0xffe0u     /* _end + /STACK:5120 */
#define PORT_HEAP_SEGMENT 0x5712u     /* first paragraph handed out by the DOS allocator */
#define PORT_ARENA_SIZE   0xa0000u    /* conventional memory: segments below A000 */

extern uint8_t *port_arena;           /* PORT_ARENA_SIZE bytes, linear = seg*16+off */
extern uint8_t *port_ds;              /* port_arena + PORT_DS_SEGMENT*16 */

/* Byte at segment:offset (fatal outside the arena). */
uint8_t *port_far(uint16_t seg, uint16_t off);
/* Host pointer of a DS offset. */
#define PORT_DS(off) (port_ds + (uint16_t)(off))
/* Little-endian word access into ds[] (16-bit wrap of the offset). */
uint16_t port_ds_word(uint16_t off);
void port_ds_set_word(uint16_t off, uint16_t value);
/* Segment:offset of a host pointer into the arena (fatal otherwise). */
uint16_t port_segment_of(const void *p);
uint16_t port_offset_of(const void *p, uint16_t seg);

/* DOS memory blocks (INT 21h AH=48h/49h semantics): paragraphs -> segment, 0 on failure. */
uint16_t port_dos_alloc(uint16_t paragraphs);
int port_dos_free(uint16_t segment);

/* Image of the recovered data modules (generated port/gen/data_image.c). */
typedef struct {
    const void *object;
    uint16_t seg, off;
    uint32_t size;
    const char *name;
} PortImageObject;
extern const PortImageObject port_image_objects[];
extern const unsigned port_image_object_count;

/* Initialized DGROUP data that code modules own in their DOS builds (string
 * literals and static tables of their _DATA segments), generated from the
 * recovered source text (port/gen/module_data.c, port/tools/gen_module_data.py). */
typedef struct {
    uint16_t seg, off;
    uint16_t size;
    const uint8_t *bytes;
    const char *module;
} PortImageBytes;
extern const PortImageBytes port_module_data[];
extern const unsigned port_module_data_count;

/* Allocate the arena and copy every recovered data object to its address. */
void port_memory_init(void);

/* ---- DGROUP stack model ------------------------------------------------ */

/* Host branches with an `sp`/`frame`/`scratch` parameter keep locals at DS
 * stack addresses. port_sp is the modeled SP (free space below it). An
 * adapter calling such a routine does
 *     uint16_t sp = port_stack_enter(PORT_FRAME);   entry SP (return-address slot)
 *     opeu_x(ds, svc_x, sp);
 *     port_stack_leave(PORT_FRAME);
 * where the reserve covers the callee's locals plus the frames of the
 * routines it calls internally with sp-n (use the module's documented frame
 * when larger). For a `frame`/`scratch` parameter that names the BP or a
 * buffer rather than the entry SP, convert as that module documents. */
#define PORT_FRAME 0x100u
extern uint16_t port_sp;
uint16_t port_stack_enter(uint16_t bytes);
void port_stack_leave(uint16_t bytes);

/* ---- routine link table ------------------------------------------------ */

/* A linked routine. Arguments are 16-bit words in the address-keyed
 * convention: register arguments in AX, DX, BX order (bytes zero-extended),
 * then stack arguments in prototype order; far pointers are (offset,
 * segment), 32-bit values (low, high). Near pointers are DS offsets. The
 * result is DX:AX (AX in the low half). */
typedef uint32_t (*PortRoutine)(uint8_t *ds, const uint16_t *args, unsigned count);

typedef struct {
    uint32_t address;       /* original linear routine address (identifier only) */
    const char *name;
    PortRoutine routine;    /* NULL: not linked yet */
} PortLink;

/* Define a linked routine handler for recovered/native routine `name`.
 * port/tools/gen_link_table.py collects every PORT_ROUTINE in port/ and
 * builds port/gen/link_table.c from the routine's address. */
#define PORT_ROUTINE(name) uint32_t port_routine_##name(uint8_t *ds, const uint16_t *args, unsigned count)

/* Call a routine by its identifier; fatal (with its name) when unlinked. */
uint32_t port_call(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count);
/* The address-keyed service callback signature used by the battle modules. */
uint16_t port_service(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count);
/* Name of a routine identifier (for diagnostics), or NULL. */
const char *port_routine_name(uint32_t address);
/* Identifier of a routine by name (the DOS symbol without _ / @ prefix), 0 if unknown. */
uint32_t port_routine_address(const char *name);

/* ---- diagnostics ------------------------------------------------------- */

/* Abort with a message (also written to port.log). Never returns. */
void port_fatal(const char *fmt, ...);
/* An event id a link adapter does not handle. Never returns. */
void port_unhandled(const char *module, unsigned id, unsigned count);
/* Trace line when OPEU_TRACE is set (category filter: OPEU_TRACE=all|link|svc|...) */
void port_trace(const char *category, const char *fmt, ...);
int port_trace_enabled(const char *category);

/* Leave the game (quit to DOS). */
void port_exit(int code);

/* ---- run-time options (port/main.c) ------------------------------------ */
typedef struct {
    int headless;            /* no window; virtual time; scripted input */
    uint32_t stop_ms;        /* headless: stop after this much virtual time (0 = never) */
    const char *png_path;    /* write the last frame here when stopping */
    const char *input_path;  /* scripted input log (see port/README.md) */
    const char *save_dir;    /* saved games (NULL: the platform's default) */
    int scale;               /* initial window scale */
    const char *shot_dir;    /* headless: write a frame every shot_every_ms here */
    uint32_t shot_every_ms;
    const char *wav_path;    /* write the audio output (49716 Hz stereo) here */
    const char *opl_log_path;/* log every OPL register write (frame, register, value) */
    int mute;                /* window mode: no audio device */
} PortOptions;
extern PortOptions port_options;

#ifdef __cplusplus
}
#endif
#endif
