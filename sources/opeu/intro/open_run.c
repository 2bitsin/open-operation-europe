/* OPEN.EXE in the port's start-up: the launcher's sequence (src/koei/
 * launcher.asm: BIOS mode 12h, FMDRV, OPEN, MAIN) in one process.
 *
 * What carries over from OPEN to MAIN is what carried over between the two
 * DOS programs: the VGA (VRAM, DAC and attribute palette: MAIN's first
 * screen is drawn in the palette OPEN faded to), the resident FM driver
 * (its song state and countdowns), the BIOS keyboard buffer and the mouse
 * driver. OPEN's memory does not: MAIN is loaded fresh (port_memory_init).
 */
#include "port.h"
#include "services.h"
#include "open.h"
#include "open_process.h"
#include "hostinput.h"                  /* port_time_us */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int intro_mode = -1;      /* -1 default, 0 --no-intro, 1 --intro */

void open_intro_options(int *argc, char **argv)
{
    int i, j;
    for (i = j = 1; i < *argc; i++) {
        if (!strcmp(argv[i], "--no-intro"))
            intro_mode = 0;
        else if (!strcmp(argv[i], "--intro"))
            intro_mode = 1;
        else
            argv[j++] = argv[i];
    }
    argv[j] = 0;
    *argc = j;
}

void open_intro_run(void)
{
    int run = intro_mode >= 0 ? intro_mode : !port_options.headless;
    if (!run)
        return;
    port_trace("svc", "OPEN.EXE: start");
    open_memory_init();
    /* crt0 -> main(): the opening's recovered main(). */
    opeu_open_main(open_ds, link_openmain);
    /* main() returned: crt0 exits, DOS frees OPEN's memory and the
     * launcher loads MAIN.EXE. */
    port_trace("svc", "OPEN.EXE: exit at %llu us", (unsigned long long)port_time_us());
    {
        /* Diagnostic (OPEU_OPEN_DUMP=file): OPEN's memory from the song
         * bank's segment on (64 KB), as the resident driver saw it at the
         * exit; port/open/tests/open_music_diff.py replays the shutdown's
         * song 5 (garbage track offsets into this memory) from it. */
        const char *path = getenv("OPEU_OPEN_DUMP");
        FILE *f = path ? fopen(path, "wb") : NULL;
        if (path && (!f || fwrite(port_arena + (uint32_t)OPEN_BANK_SEGMENT * 16u, 1, 0x10000, f) != 0x10000))
            port_fatal("cannot write %s", path);
        if (f)
            fclose(f);
    }
    port_memory_init();
}
