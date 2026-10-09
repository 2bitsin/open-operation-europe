/* FILEIO (src/main/asm/fileio.asm, _TEXT 0000..011F): the KOEI DOS file and
 * memory wrappers, natively over the platform file API.
 *
 * DOS handles: 0..4 are the predefined devices; files get the lowest free
 * handle from 5 up to 19 (the 20-entry job file table of DOS). Results
 * follow the assembly exactly:
 *  - open/create: the handle, or -1 when DOS sets CF;
 *  - close: always 0 (CF is discarded);
 *  - read/write (near and far): raw AX, i.e. the byte count, or the DOS
 *    error code when DOS failed (6 = invalid handle; CF is ignored);
 *  - seek: DX:AX, the new position; on error AX = the DOS error code and DX
 *    keeps the value it was loaded with (the position's low word);
 *  - exists: open(name, 0) and close; 1 when both succeed, else 0;
 *  - alloc: the segment or 0; free: 1 on success, 0 on failure.
 *
 * Open mode (AL of 3Dh): 0 read, 1 write, 2 read/write. Read/write opens
 * fall back to read-only when the file cannot be opened for writing (the
 * original game directory may be read-only).
 */
#include "port.h"
#include "services.h"
#include "platform.h"

#include <stdio.h>
#include <string.h>

#define DOS_HANDLES 20
#define DOS_FIRST_FILE 5

#define DOS_ERR_NOT_FOUND 2
#define DOS_ERR_INVALID_HANDLE 6
#define DOS_ERR_ACCESS 5

static struct {
    void *file;
    uint16_t mode;          /* 0 read, 1 write, 2 read/write */
    int last_write;         /* the previous transfer was a write */
} handles[DOS_HANDLES];

static int16_t new_handle(void *file, uint16_t mode)
{
    int h;
    for (h = DOS_FIRST_FILE; h < DOS_HANDLES; h++)
        if (!handles[h].file) {
            handles[h].file = file;
            handles[h].mode = mode;
            handles[h].last_write = 0;
            return (int16_t)h;
        }
    plat_fclose(file);      /* DOS error 4: too many open files */
    return -1;
}

static int valid(int16_t h)
{
    return h >= DOS_FIRST_FILE && h < DOS_HANDLES && handles[h].file != NULL;
}

int16_t opeu_dos_open(const char *name, uint16_t mode)
{
    void *f = NULL;
    uint16_t access = (uint16_t)(mode & 7);
    int16_t h;
    switch (access) {
    case 0: f = plat_fopen(name, "rb"); break;
    case 1:
    case 2:
        f = plat_fopen(name, "r+b");
        if (!f) {
            f = plat_fopen(name, "rb");
            if (f)
                port_trace("svc", "dos_open %s: read/write refused, opened read-only", name);
        }
        break;
    default: break;
    }
    h = f ? new_handle(f, access) : -1;
    port_trace("svc", "dos_open(%s, %u) = %d", name, mode, h);
    return h;
}

int16_t opeu_dos_close(int16_t handle)
{
    if (valid(handle)) {
        plat_fclose(handles[handle].file);
        handles[handle].file = NULL;
    }
    port_trace("svc", "dos_close(%d)", handle);
    return 0;
}

/* stdio requires a positioning call between reads and writes on a stream. */
static void switch_direction(int16_t handle, int writing)
{
    if (handles[handle].last_write != writing) {
        plat_fseek(handles[handle].file, 0, SEEK_CUR);
        handles[handle].last_write = writing;
    }
}

static int16_t do_read(int16_t handle, uint8_t *data, uint16_t size)
{
    size_t n;
    if (!valid(handle))
        return DOS_ERR_INVALID_HANDLE;
    if (handles[handle].mode == 1)
        return DOS_ERR_ACCESS;
    switch_direction(handle, 0);
    n = plat_fread(data, size, handles[handle].file);
    return (int16_t)(uint16_t)n;
}

static int16_t do_write(int16_t handle, const uint8_t *data, uint16_t size)
{
    size_t n;
    if (!valid(handle))
        return DOS_ERR_INVALID_HANDLE;
    if (handles[handle].mode == 0)
        return DOS_ERR_ACCESS;
    switch_direction(handle, 1);
    if (size == 0)
        return 0;   /* DOS truncates at the position; nothing MAIN relies on */
    n = plat_fwrite(data, size, handles[handle].file);
    return (int16_t)(uint16_t)n;
}

int16_t opeu_dos_read(int16_t handle, uint8_t *data, uint16_t size)
{
    int16_t r = do_read(handle, data, size);
    port_trace("svc", "dos_read(%d, %u) = %d", handle, size, r);
    return r;
}

int16_t opeu_dos_read_far(int16_t handle, uint16_t segment, uint16_t offset, uint16_t size)
{
    int16_t r;
    if (size)
        (void)port_far(segment, (uint16_t)(offset + size - 1));   /* range check */
    r = do_read(handle, port_far(segment, offset), size);
    port_trace("svc", "dos_read_far(%d, %04x:%04x, %u) = %d", handle, segment, offset, size, r);
    return r;
}

int16_t opeu_dos_write(int16_t handle, const uint8_t *data, uint16_t size)
{
    int16_t r = do_write(handle, data, size);
    port_trace("svc", "dos_write(%d, %u) = %d", handle, size, r);
    return r;
}

int16_t opeu_dos_write_far(int16_t handle, uint16_t segment, uint16_t offset, uint16_t size)
{
    int16_t r;
    if (size)
        (void)port_far(segment, (uint16_t)(offset + size - 1));
    r = do_write(handle, port_far(segment, offset), size);
    port_trace("svc", "dos_write_far(%d, %04x:%04x, %u) = %d", handle, segment, offset, size, r);
    return r;
}

int32_t opeu_dos_seek(int16_t handle, int32_t position, int16_t origin)
{
    long r;
    uint16_t low = (uint16_t)position;
    uint8_t al = (uint8_t)origin;
    if (!valid(handle))
        return (int32_t)((uint32_t)low << 16 | DOS_ERR_INVALID_HANDLE);
    if (al > 2)
        return (int32_t)((uint32_t)low << 16 | 1);   /* invalid function */
    /* DOS takes CX:DX as a signed 32-bit offset for origins 1 and 2 and as
     * unsigned for 0. */
    r = plat_fseek(handles[handle].file,
                   al == 0 ? (long)(uint32_t)position : (long)position,
                   al == 0 ? SEEK_SET : al == 1 ? SEEK_CUR : SEEK_END);
    handles[handle].last_write = 0;
    port_trace("svc", "dos_seek(%d, %ld, %d) = %ld", handle, (long)position, origin, r);
    if (r < 0)
        return (int32_t)((uint32_t)low << 16 | 0x19);  /* seek error */
    return (int32_t)(uint32_t)r;
}

int16_t opeu_dos_create(const char *name)
{
    void *f = plat_fopen(name, "w+b");
    int16_t h = f ? new_handle(f, 2) : -1;
    port_trace("svc", "dos_create(%s) = %d", name, h);
    return h;
}

int16_t opeu_dos_exists(const char *name)
{
    int16_t h = opeu_dos_open(name, 0);
    if (h == -1)
        return 0;
    if (opeu_dos_close(h) == -1)
        return 0;
    return 1;
}

uint16_t opeu_dos_alloc(uint16_t paragraphs)
{
    return port_dos_alloc(paragraphs);
}

int16_t opeu_dos_free(uint16_t segment)
{
    return port_dos_free(segment) == 0 ? 1 : 0;
}

/* INT 21h AH=0Dh: DOS writes its dirty buffers; flush every open stream. */
void port_disk_reset(void)
{
    int h;
    for (h = DOS_FIRST_FILE; h < DOS_HANDLES; h++)
        if (handles[h].file)
            fflush((FILE *)handles[h].file);
}
