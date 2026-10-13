// The opening in the program's start-up: the launcher's sequence (BIOS mode 12h, the FM driver, the opening, the game)
// in one process.
//
// What carries over from the opening to the game is what carried over between the two DOS programs: the VGA (VRAM, DAC
// and attribute palette: the game's first screen is drawn in the palette the opening faded to), the resident FM driver
// (its song state and countdowns), the BIOS keyboard buffer and the mouse driver. The opening's memory does not: the
// game's image is loaded fresh (port_memory_init).
#include "hostinput.hpp" // port_time_us
#include "intro.hpp"
#include "open-memory.hpp"
#include "open.hpp"
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>

namespace opeu::intro {
namespace {
constexpr std::string_view INTRO_FLAG{ "--intro" };
constexpr std::string_view NO_INTRO_FLAG{ "--no-intro" };

// --intro or --no-intro, the last one given; neither: the opening runs in a window only. Set by the entry's option
// pass before the opening runs.
std::optional<bool> requested; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables): the entry's two calls share it

auto IsIntroFlag(char const* argument) -> bool {
  return argument == INTRO_FLAG || argument == NO_INTRO_FLAG;
}

// Diagnostic (OPEU_OPEN_DUMP=file): the opening's memory from the song bank's segment on (64 KiB), as the resident
// driver saw it at the exit, to replay the shutdown's song 5 (whose track offsets point into this memory) from it.
auto DumpBankSegment(OpenMemory const& memory) -> void {
  char const* const path{ std::getenv("OPEU_OPEN_DUMP") }; // NOLINT(concurrency-mt-unsafe): read once at start-up
  if (path == nullptr) return;
  std::ofstream file{ path, std::ios::binary };
  auto const    bank{ std::as_bytes(memory.Segment(OPEN_BANK_SEGMENT)) };
  file.write(reinterpret_cast<char const*>(bank.data()), static_cast<std::streamsize>(bank.size())); // NOLINT(*-reinterpret-cast): bytes out
  if (!file) port_fatal("cannot write %s", path);
}

// The opening as DOS ran it: its image loaded into the conventional memory, its main() run to the end.
auto RunOpenExe() -> void {
  port_arena_prepare();
  OpenMemory memory{
    std::span{ port_arena, PORT_ARENA_SIZE + SEGMENT_SIZE }
  };
  memory.Load();
  OpenState         state{ memory.Segment(OPEN_DS_SEGMENT) };
  services::Machine machine;
  Open              open{ state, memory, machine };
  RunOpening(open);
  port_trace("svc", "OPEN.EXE: exit at %llu us", static_cast<unsigned long long>(port_time_us()));
  DumpBankSegment(memory);
}
} // namespace

// Takes --intro and --no-intro out of the command line; the other arguments keep their order, argv its terminating null.
void open_intro_options(int* argc, char** argv) {
  auto const      count{ static_cast<std::size_t>(*argc) };
  std::span const arguments{ argv, count + 1 }; // argv[argc] is the terminating null
  auto const      options{ arguments.first(count).subspan(count > 0 ? 1 : 0) };
  std::ranges::for_each(options | std::views::filter(IsIntroFlag), [](char const* flag) { requested = flag == INTRO_FLAG; });
  auto const kept{ count - std::ranges::remove_if(options, IsIntroFlag).size() };
  arguments[kept] = nullptr;
  *argc           = static_cast<int>(kept);
}

void open_intro_run() {
  if (!requested.value_or(port_options.headless == 0)) return;
  port_trace("svc", "OPEN.EXE: start");
  RunOpenExe();
  port_memory_init();
}

}
