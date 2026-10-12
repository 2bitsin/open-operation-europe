/* OPEN.EXE in the port's start-up: the launcher's sequence (KOEI.COM: BIOS
 * mode 12h, FMDRV, OPEN, MAIN) in one process.
 *
 * What carries over from OPEN to MAIN is what carried over between the two
 * DOS programs: the VGA (VRAM, DAC and attribute palette: MAIN's first
 * screen is drawn in the palette OPEN faded to), the resident FM driver
 * (its song state and countdowns), the BIOS keyboard buffer and the mouse
 * driver. OPEN's memory does not: MAIN is loaded fresh (port_memory_init).
 */
#include "hostinput.hpp" /* port_time_us */
#include "intro.hpp"
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace opeu::intro {
namespace {
int intro_mode = -1; /* -1 default, 0 --no-intro, 1 --intro */
}

void open_intro_options(int* argc, char** argv) {
  int i = 0;
  int j = 0;
  for (i = j = 1; i < *argc; i++) {
    if (strcmp(argv[i], "--no-intro") == 0)
      intro_mode = 0;
    else if (strcmp(argv[i], "--intro") == 0)
      intro_mode = 1;
    else
      argv[j++] = argv[i];
  }
  argv[j] = nullptr;
  *argc   = j;
}

void open_intro_run() {
  int run = intro_mode >= 0 ? intro_mode : static_cast<int>(port_options.headless == 0);
  if (run == 0) return;
  port_trace("svc", "OPEN.EXE: start");
  open_memory_init();
  /* crt0 -> main(): the opening's recovered main(). */
  opeu_open_main(open_ds, link_openmain);
  /* main() returned: crt0 exits, DOS frees OPEN's memory and the
     * launcher loads MAIN.EXE. */
  port_trace("svc", "OPEN.EXE: exit at %llu us", static_cast<unsigned long long>(port_time_us()));
  {
    /* Diagnostic (OPEU_OPEN_DUMP=file): OPEN's memory from the song
         * bank's segment on (64 KB), as the resident driver saw it at the
         * exit, to replay the shutdown's song 5 (garbage track offsets into
         * this memory) from it. */
    if (char const* const path{ getenv("OPEU_OPEN_DUMP") }; path != nullptr) {
      std::ofstream file{ path, std::ios::binary };
      file.write(reinterpret_cast<char const*>(port_arena) + (static_cast<uint32_t>(OPEN_BANK_SEGMENT) * 16u), 0x10000);
      if (!file) port_fatal("cannot write %s", path);
    }
  }
  port_memory_init();
}

}
