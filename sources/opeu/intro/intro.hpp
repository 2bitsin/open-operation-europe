// The opening program OPEN.EXE in the native port.
//
// The original launcher (KOEI.COM) sets BIOS mode 12h, runs FMDRV.COM (resident), then OPEN.EXE, then MAIN.EXE. The
// port does the same in one process: the entry installs the FM driver, RunIntro runs the opening (RunOpening,
// recovered/scenes.cpp) and then the game starts with the same window, VGA state and driver.
//
//   IntroOptions(arguments)  takes --no-intro / --intro out of the command line (before the entry's own parser)
//   RunIntro(run)            the opening, unless disabled; afterwards the data arena holds the game's initial image
//
// The opening runs by default in a window; headless runs (--headless) skip it unless --intro is given, so the scripted
// runs, their PNGs and their audio keep their timings.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace opeu::intro::detail::intro {

// What the command line asks of the opening: the last of --intro and --no-intro, or neither.
enum class IntroRequest : std::uint8_t {
  IN_A_WINDOW_ONLY, // neither: the opening runs in a window, not headless
  RUN,              // --intro
  SKIP,             // --no-intro
};

// The opening's request and the arguments left for the entry's parser, in their order (the program name first).
struct IntroCommandLine {
  IntroRequest                  request{ IntroRequest::IN_A_WINDOW_ONLY };
  std::vector<std::string_view> remaining;
};

// How the opening runs: whether (with the run's presentation), and where its song-bank memory is written at the exit
// (a diagnostic, OPEU_OPEN_DUMP), if anywhere.
struct IntroRun {
  IntroRequest                         request{ IntroRequest::IN_A_WINDOW_ONLY };
  std::optional<std::filesystem::path> bank_dump;
};

[[nodiscard]] auto IntroOptions(std::span<std::string_view const> arguments) -> IntroCommandLine;
// The opening as the launcher ran it, when the run asks for it.
auto RunIntro(IntroRun const& run) -> void;

}

namespace opeu::intro {
using detail::intro::IntroCommandLine;
using detail::intro::IntroOptions;
using detail::intro::IntroRequest;
using detail::intro::IntroRun;
using detail::intro::RunIntro;
}
