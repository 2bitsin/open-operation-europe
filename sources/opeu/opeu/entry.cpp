// Operation Europe: the program entry. The host prepares what the DOS start-up and the launcher provided (the
// initialised data segment, the memory arena, the display, input and the FM driver), runs the opening, then calls the
// game's application entry with its service table.
#include "common/game.hpp"
#include "core/app.hpp"
#include "intro.hpp"
#include "link/glue.hpp"
#include "machine.hpp"
#include "native-machine.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <print>
#include <span>
#include <string_view>
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#include <unistd.h>
#endif

namespace opeu::app::detail::entry {
namespace {
auto Usage(std::string_view argv0) -> void {
  std::print(stderr,
             "usage: {} [--saves DIR] [--scale N] [--headless] [--ms N] [--input FILE] [--png FILE] [--wav FILE]\n"
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
// Diagnostic (OPEU_BACKTRACE): print the native call stack on a crash.
auto CrashBacktrace(int sig) -> void {
  std::array<void*, 64>       frames{};
  int const                   count{ backtrace(frames.data(), static_cast<int>(frames.size())) };
  constexpr std::string_view  message{ "opeu: fatal signal, native backtrace:\n" };
  [[maybe_unused]] auto const written{ write(2, message.data(), message.size()) };
  backtrace_symbols_fd(frames.data(), count, 2);
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}
#endif

auto ShutdownPlatform() -> void {
  plat_shutdown();
}

auto Milliseconds(char const* text) -> std::uint32_t {
  return static_cast<std::uint32_t>(std::strtoul(text, nullptr, 0));
}

// The command line into port_options; the arena dump path, if any, into `dump_path`. False on a bad option.
auto ParseOptions(std::span<char*> args, char const*& dump_path) -> bool {
  for (auto at{ args.begin() + 1 }; at != args.end(); ++at) {
    std::string_view const option{ *at };
    auto const             values{ args.end() - at - 1 };
    auto                   next{ [&at] -> char* { return *++at; } };
    if (option == "--saves" && values >= 1) {
      port_options.save_dir = next();
    } else if (option == "--scale" && values >= 1) {
      port_options.scale = std::atoi(next());
    } else if (option == "--headless") {
      port_options.headless = 1;
    } else if (option == "--ms" && values >= 1) {
      port_options.stop_ms = Milliseconds(next());
    } else if (option == "--input" && values >= 1) {
      port_options.input_path = next();
    } else if (option == "--png" && values >= 1) {
      port_options.png_path = next();
    } else if (option == "--shots" && values >= 2) {
      port_options.shot_dir      = next();
      port_options.shot_every_ms = Milliseconds(next());
    } else if (option == "--wav" && values >= 1) {
      port_options.wav_path = next();
    } else if (option == "--opl-log" && values >= 1) {
      port_options.opl_log_path = next();
    } else if (option == "--mute") {
      port_options.mute = 1;
    } else if (option == "--dump-arena" && values >= 1) {
      dump_path             = next();
      port_options.headless = 1;
    } else {
      return false;
    }
  }
  return true;
}

// Diagnostic: the initial far-data arena (the data segments as composed from the game's data modules).
auto DumpArena(char const* path) -> void {
  std::ofstream file{ path, std::ios::binary };
  file.write(reinterpret_cast<char const*>(port_arena), PORT_ARENA_SIZE); // NOLINT(*-reinterpret-cast): bytes out
  if (!file) port_fatal("cannot write %s", path);                         // NOLINT(*-pro-type-vararg): the glue's printf-shaped fatal exit
}
// The game's application entry on the data segment the arena holds and the machine it runs on, the native devices
// (adapters over the services) wired in here; it returns after the scenario's end.
auto RunGame() -> void {
  opeu::game::GameState state{
    opeu::game::SegmentBytes{ port_ds, opeu::game::SEGMENT_SIZE }
  };
  opeu::services::NativeDevices devices;
  opeu::services::Machine       machine{ devices.Devices() };
  opeu::game::Game              game{ state, machine, opeu::game::UiHost{ .memory = std::span{ port_arena, PORT_ARENA_SIZE } } };
  opeu_application_entry(game);
}
} // namespace

auto Main(int argc, char** argv) -> int {
  char const* dump_path{ nullptr };

  port_options.scale = 2;
  opeu::intro::open_intro_options(&argc, argv); // --no-intro / --intro
  std::span<char*> const args{ argv, static_cast<std::size_t>(argc) };
  if (!ParseOptions(args, dump_path)) {
    Usage(args.front());
    return 2;
  }

#if defined(__APPLE__) || defined(__linux__)
  if (std::getenv("OPEU_BACKTRACE") != nullptr) {
    std::signal(SIGSEGV, CrashBacktrace);
    std::signal(SIGBUS, CrashBacktrace);
    std::signal(SIGABRT, CrashBacktrace);
  }
#endif
  PlatConfig const config{
    .title      = "Operation Europe",
    .width      = 640,
    .height     = 480,
    .scale      = port_options.scale > 0 ? port_options.scale : 2,
    .headless   = port_options.headless,
    .fullscreen = 0,
  };
  if (port_options.save_dir != nullptr) plat_set_save_dir(port_options.save_dir);
  if (plat_init(&config) != 0) return 1;
  std::atexit(ShutdownPlatform);

  vgalib_init();
  port_memory_init();
  if (dump_path != nullptr) {
    DumpArena(dump_path);
    return 0;
  }

  // The FM driver was resident before the game started.
  port_fm_init();
  // The launcher ran the opening between the FM driver and the game.
  opeu::intro::open_intro_run();
  RunGame();
  port_exit(0);
  return 0;
}
} // namespace opeu::app::detail::entry

// main.cpp of the application module calls this.
auto opeu_main(int argc, char** argv) -> int {
  return opeu::app::detail::entry::Main(argc, argv);
}
