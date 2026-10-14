// The opening in the program's start-up: the launcher's sequence (BIOS mode 12h, the FM driver, the opening, the game)
// in one process.
//
// What carries over from the opening to the game is what carried over between the two DOS programs: the VGA (VRAM, DAC
// and attribute palette: the game's first screen is drawn in the palette the opening faded to), the resident FM driver
// (its song state and countdowns), the BIOS keyboard buffer and the mouse driver. The opening's memory does not: the
// game's image is loaded fresh (port_memory_init).
#include "intro.hpp"

#include "hostinput.hpp" // port_time_us
#include "load-image.hpp"
#include "open-memory.hpp"
#include "open.hpp"
#include "port.hpp"
#include "recovered.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::intro::detail::intro {
namespace {
constexpr std::string_view INTRO_FLAG{ "--intro" };
constexpr std::string_view NO_INTRO_FLAG{ "--no-intro" };

auto IsIntroFlag(std::string_view argument) -> bool {
  return argument == INTRO_FLAG || argument == NO_INTRO_FLAG;
}

auto RequestOf(std::string_view flag) -> IntroRequest {
  return flag == INTRO_FLAG ? IntroRequest::RUN : IntroRequest::SKIP;
}

auto Wanted(IntroRequest request) -> bool {
  switch (request) {
  case IntroRequest::RUN:              return true;
  case IntroRequest::SKIP:             return false;
  case IntroRequest::IN_A_WINDOW_ONLY: return port_options.headless == 0;
  }
  return false;
}

// Diagnostic: the opening's memory from the song bank's segment on (64 KiB), as the resident driver saw it at the exit,
// to replay the shutdown's song 5 (whose track offsets point into this memory) from it.
auto DumpBankSegment(OpenMemory const& memory, std::filesystem::path const& path) -> void {
  std::ofstream file{ path, std::ios::binary };
  auto const    bank{ std::as_bytes(memory.Segment(OPEN_BANK_SEGMENT)) };
  file.write(reinterpret_cast<char const*>(bank.data()), static_cast<std::streamsize>(bank.size())); // NOLINT(*-reinterpret-cast): bytes out
  if (!file) port_fatal("cannot write %s", path.string().c_str());
}

// The opening as DOS ran it: its image loaded into the conventional memory, its main() run to the end.
auto RunOpenExe(std::optional<std::filesystem::path> const& bank_dump) -> void {
  port_arena_prepare();
  OpenMemory memory{
    std::span{ port_arena, PORT_ARENA_SIZE + SEGMENT_SIZE }
  };
  memory.Load();
  OpenState         state{ memory.Segment(OPEN_DS_SEGMENT) };
  services::Machine machine;
  Open              open{ state, memory, machine };
  RunOpening(open);
  port_trace("svc", "%s", std::format("OPEN.EXE: exit at {} us", port_time_us()).c_str());
  if (bank_dump) DumpBankSegment(memory, *bank_dump);
}
} // namespace

auto IntroOptions(std::span<std::string_view const> const arguments) -> IntroCommandLine {
  auto const options{ arguments | std::views::drop(1) };
  auto       flags{ options | std::views::reverse | std::views::filter(IsIntroFlag) };
  auto const request{ flags.empty() ? IntroRequest::IN_A_WINDOW_ONLY : RequestOf(flags.front()) };
  auto       remaining{ arguments | std::views::take(1) | std::ranges::to<std::vector>() };
  std::ranges::copy(options | std::views::filter(std::not_fn(IsIntroFlag)), std::back_inserter(remaining));
  return { .request = request, .remaining = std::move(remaining) };
}

auto RunIntro(IntroRun const& run) -> void {
  if (!Wanted(run.request)) return;
  port_trace("svc", "OPEN.EXE: start");
  RunOpenExe(run.bank_dump);
  port_memory_init();
}

} // namespace opeu::intro::detail::intro
