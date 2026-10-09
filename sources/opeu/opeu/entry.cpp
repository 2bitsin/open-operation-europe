/* Operation Europe, native port: program entry.
 *
 * The original start-up is MS C's crt0 calling main(), which is the
 * recovered application entry (core/app.c, opeu_application_entry at
 * 0977:03dc). Here the host prepares what crt0 and DOS provided (the
 * initialized data segment, the memory arena, the display and input
 * devices) and then calls the same entry with app.c's service table.
 */
#include "core/app.hpp"
#include "glue.hpp"
#include "intro.hpp"                    /* OPEN.EXE, the opening (port/open) */
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#include <unistd.h>
#endif

static void usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [--saves DIR] [--scale N] [--headless] [--ms N] [--input FILE] [--png FILE] [--wav FILE]\n"
          "  --saves DIR   saved games (default: the per-user data directory)\n"
          "  --headless    no window; virtual time; use with --ms/--input/--png\n"
          "  --ms N        headless: stop after N ms of virtual time\n"
          "  --input FILE  scripted input log (see docs/port.md)\n"
          "  --png FILE    write the final frame as PNG\n"
          "  --wav FILE    write the music and sound (49716 Hz stereo WAV)\n"
          "  --opl-log F   log every OPL2 register write (frame, register, value)\n"
          "  --mute        no audio device\n",
          argv0);
}

#if defined(__APPLE__) || defined(__linux__)
/* Diagnostic (OPEU_BACKTRACE): print the native call stack on a crash. */
static void crash_backtrace(int sig) {
  void* frames[64];
  int n = backtrace(frames, 64);
  static const char msg[] = "opeu: fatal signal, native backtrace:\n";
  (void)!write(2, msg, sizeof msg - 1);
  backtrace_symbols_fd(frames, n, 2);
  signal(sig, SIG_DFL);
  raise(sig);
}
#endif

static void shutdown_platform(void) {
  plat_shutdown();
}

/* main.cpp of the application module (sources/opeu/opeu) calls this. */
int opeu_main(int argc, char** argv) {
  PlatConfig cfg;
  const char* dump_path = 0;
  int i;

  port_options.scale = 2;
  open_intro_options(&argc, argv);     /* --no-intro / --intro */
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--saves") && i + 1 < argc)
      port_options.save_dir = argv[++i];
    else if (!strcmp(argv[i], "--scale") && i + 1 < argc)
      port_options.scale = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--headless"))
      port_options.headless = 1;
    else if (!strcmp(argv[i], "--ms") && i + 1 < argc)
      port_options.stop_ms = (uint32_t)strtoul(argv[++i], 0, 0);
    else if (!strcmp(argv[i], "--input") && i + 1 < argc)
      port_options.input_path = argv[++i];
    else if (!strcmp(argv[i], "--png") && i + 1 < argc)
      port_options.png_path = argv[++i];
    else if (!strcmp(argv[i], "--shots") && i + 2 < argc) {
      port_options.shot_dir = argv[++i];
      port_options.shot_every_ms = (uint32_t)strtoul(argv[++i], 0, 0);
    } else if (!strcmp(argv[i], "--wav") && i + 1 < argc)
      port_options.wav_path = argv[++i];
    else if (!strcmp(argv[i], "--opl-log") && i + 1 < argc)
      port_options.opl_log_path = argv[++i];
    else if (!strcmp(argv[i], "--mute"))
      port_options.mute = 1;
    else if (!strcmp(argv[i], "--dump-arena") && i + 1 < argc) {
      dump_path = argv[++i];
      port_options.headless = 1;
    } else {
      usage(argv[0]);
      return 2;
    }
  }

#if defined(__APPLE__) || defined(__linux__)
  if (getenv("OPEU_BACKTRACE")) {
    signal(SIGSEGV, crash_backtrace);
    signal(SIGBUS, crash_backtrace);
    signal(SIGABRT, crash_backtrace);
  }
#endif
  memset(&cfg, 0, sizeof cfg);
  cfg.title = "Operation Europe";
  cfg.width = 640;
  cfg.height = 480;
  cfg.scale = port_options.scale > 0 ? port_options.scale : 2;
  cfg.headless = port_options.headless;
  if (port_options.save_dir) plat_set_save_dir(port_options.save_dir);
  if (plat_init(&cfg) != 0) return 1;
  atexit(shutdown_platform);

  vgalib_init();
  port_memory_init();
  if (dump_path) {
    /* Diagnostic: the initial far-data arena (FAR_DATA segments and
         * DGROUP as composed from the recovered data modules). */
    FILE* f = fopen(dump_path, "wb");
    if (!f || fwrite(port_arena, 1, PORT_ARENA_SIZE, f) != PORT_ARENA_SIZE) port_fatal("cannot write %s", dump_path);
    fclose(f);
    return 0;
  }

  /* The boundary pointer the AI host branches expect the embedding driver
     * to install (opeu_ai_unrecovered, game/core/aiops1.c). */
  port_install_hooks();

  /* FMDRV was installed by OPEU.BAT before MAIN started. */
  port_fm_init();

  /* KOEI.COM ran OPEN.EXE between FMDRV and MAIN (port/open). */
  open_intro_run();

  /* crt0 -> main(): the recovered application entry. */
  opeu_application_entry(port_ds, svc_app);

  /* The entry returns after the scenario end (FINISH); crt0 then exits 0,
     * which made the launcher run the ending program. */
  port_exit(0);
  return 0;
}
