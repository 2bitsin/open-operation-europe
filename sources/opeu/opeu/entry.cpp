// Operation Europe: the program entry. It finds the game's data files (data-choice.hpp: --data, the files compiled in,
// or the search), then the host prepares what the DOS start-up and the launcher provided (the initialised data segment,
// the memory arena, the display, input and the FM driver), runs the opening, then calls the game's application entry
// on its machine: the game's screen on SDL_Renderer over the classic pack (game-screen.hpp) and the native files,
// input, sound and clock.
#include "common/game.hpp"
#include "core/app.hpp"
#include "data-choice.hpp"
#include "embedded.hpp"
#include "game-screen.hpp"
#include "intro.hpp"
#include "link/image.hpp"
#include "machine.hpp"
#include "native-machine.hpp"
#include "opeu/assets/game-data.hpp"
#include "opeu/platform/data-places.hpp"
#include "opeu/services/asset-table.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"
#include "shown-frames.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#include <unistd.h>
#endif

namespace opeu::app::detail::entry {
namespace {
constexpr auto NO_GAME_DATA{ 3 }; // the exit code when the game's data files are missing or not the release's

auto Usage(std::string_view argv0) -> void {
  std::print(stderr,
             "usage: {} [--data DIR] [--check-data [DIR]] [--remaster PATH] [--saves DIR] [--scale N] [--headless] [--ms N]\n"
             "          [--input FILE] [--png FILE] [--wav FILE]\n"
             "  --data DIR    the game's 26 data files are in DIR (default: compiled in, else searched; docs/building.md)\n"
             "  --check-data [DIR]  say whether DIR (default: where the run would look) holds the data files, and stop\n"
             "  --remaster PATH  draw with the remaster pack at PATH, a directory or .opeupack (default: $OPEU_REMASTER)\n"
             "  --saves DIR   saved games (default: the per-user data directory)\n"
             "  --scale N     open the window at N times 640x480 device pixels (default: the largest the screen holds)\n"
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

// The environment variable `name`, if it is set (read at start-up, before any thread).
auto Environment(char const* name) -> std::optional<std::string_view> {
  auto const* const value{ std::getenv(name) };
  if (value == nullptr) return std::nullopt;
  return std::string_view{ value };
}

// Diagnostic (OPEU_OPEN_DUMP=file): where the opening writes its song bank's memory at the exit.
auto BankDumpPath() -> std::optional<std::filesystem::path> {
  return Environment("OPEU_OPEN_DUMP").transform([](std::string_view path) { return std::filesystem::path{ path }; });
}

auto ShutdownPlatform() -> void {
  plat_shutdown();
}

// A number in decimal, or in hexadecimal after 0x; 0 when the text is not one.
template <class _Number>
auto NumberIn(std::string_view text) -> _Number {
  auto const hexadecimal{ text.starts_with("0x") || text.starts_with("0X") };
  auto const digits{ hexadecimal ? text.substr(2) : text };
  auto       number{ _Number{} };
  std::ignore = std::from_chars(std::to_address(digits.begin()), std::to_address(digits.end()), number, hexadecimal ? 16 : 10);
  return number;
}

auto Milliseconds(std::string_view text) -> std::uint32_t {
  return NumberIn<std::uint32_t>(text);
}

auto Number(std::string_view text) -> int {
  return NumberIn<int>(text);
}

// What the command line asks of the entry itself; the run's other options go into the glue's options.
struct EntryOptions {
  std::optional<std::string_view> dump_path; // --dump-arena: write the initial arena there and stop
};

// Where the game's data and remaster come from, as the command line says, and the arguments left for ParseOptions.
struct DataCommandLine {
  std::optional<std::string_view> data;       // --data, or --check-data's directory
  std::optional<std::string_view> remaster;   // --remaster
  bool                            check_data; // --check-data: report on the data files and stop
  std::vector<std::string_view>   remaining;  // the program name first
};

// DataOptions' scan of the command line: what it has found, and the option whose value the next argument is.
struct DataScan {
  DataCommandLine  found;
  std::string_view awaiting;
};

// One argument of the scan.
auto Scanned(DataScan scan, std::string_view const argument) -> DataScan {
  auto const awaiting{ std::exchange(scan.awaiting, std::string_view{}) };
  if (awaiting == "--data" || (awaiting == "--check-data" && !argument.starts_with("--"))) {
    scan.found.data = argument;
  } else if (awaiting == "--remaster") {
    scan.found.remaster = argument;
  } else if (argument == "--data" || argument == "--remaster" || argument == "--check-data") {
    scan.found.check_data = scan.found.check_data || argument == "--check-data";
    scan.awaiting         = argument;
  } else {
    scan.found.remaining.push_back(argument);
  }
  return scan;
}

// Takes --data DIR, --check-data [DIR] and --remaster PATH out of the command line; an option without its value stays
// for ParseOptions, which refuses it.
auto DataOptions(std::span<std::string_view const> const args) -> DataCommandLine {
  auto const start{
    DataScan{ .found = { .data = {}, .remaster = {}, .check_data = false, .remaining = { args.front() } }, .awaiting = {} }
  };
  auto scan{ std::ranges::fold_left(args | std::views::drop(1), start, Scanned) };
  if (scan.awaiting == "--data" || scan.awaiting == "--remaster") scan.found.remaining.emplace_back(scan.awaiting);
  return std::move(scan.found);
}

// The command line into the glue's run options (its text fields point into argv, which outlives the run); nothing on a
// bad option.
auto ParseOptions(std::span<std::string_view const> args) -> std::optional<EntryOptions> {
  auto entry{ EntryOptions{} };
  // A hand loop: an option consumes the values after it.
  for (auto at{ args.begin() + 1 }; at != args.end(); ++at) {
    std::string_view const option{ *at };
    auto const             values{ args.end() - at - 1 };
    auto                   next{ [&at] -> std::string_view { return *++at; } };
    if (option == "--saves" && values >= 1) {
      port_options.save_dir = next().data();
    } else if (option == "--scale" && values >= 1) {
      port_options.scale = Number(next());
    } else if (option == "--headless") {
      port_options.headless = 1;
    } else if (option == "--ms" && values >= 1) {
      port_options.stop_ms = Milliseconds(next());
    } else if (option == "--input" && values >= 1) {
      port_options.input_path = next().data();
    } else if (option == "--png" && values >= 1) {
      port_options.png_path = next().data();
    } else if (option == "--shots" && values >= 2) {
      port_options.shot_dir      = next().data();
      port_options.shot_every_ms = Milliseconds(next());
    } else if (option == "--wav" && values >= 1) {
      port_options.wav_path = next().data();
    } else if (option == "--opl-log" && values >= 1) {
      port_options.opl_log_path = next().data();
    } else if (option == "--mute") {
      port_options.mute = 1;
    } else if (option == "--dump-arena" && values >= 1) {
      entry.dump_path       = next();
      port_options.headless = 1;
    } else {
      return std::nullopt;
    }
  }
  return entry;
}

// Diagnostic: the initial conventional memory (the far data and the data segment as composed from the game's data
// modules).
auto DumpArena(std::filesystem::path const& path) -> void {
  std::ofstream file{ path, std::ios::binary };
  auto const    image{ opeu::game::InitialArenaImage() };
  file.write(reinterpret_cast<char const*>(image.data()), static_cast<std::streamsize>(image.size())); // NOLINT(*-reinterpret-cast): bytes out
  if (!file) port_fatal("cannot write %s", path.string().c_str());                                     // NOLINT(*-pro-type-vararg): the glue's printf-shaped fatal exit
}

// The native devices with `display` in place of the native one.
auto WithDisplay(opeu::services::Devices const native, opeu::services::Display& display) -> opeu::services::Devices {
  return { .display = display, .files = native.files, .input = native.input, .sound = native.sound, .clock = native.clock };
}

// The game's application entry on its initial state and the machine it runs on: the game's screen (with the remaster
// at `remaster`, if any), which the pump shows from now on, and the native files, input, sound and clock (adapters over
// the services); it returns after the scenario's end. The screen's error when it cannot be opened.
auto RunGame(opeu::assets::GameData const& data, std::optional<std::filesystem::path> const& remaster) -> std::expected<void, std::string> {
  auto       state{ opeu::game::GameState::Initial() };
  auto const screen{ GameScreen::Open(data, remaster) };
  if (!screen) return std::unexpected{ screen.error() };
  opeu::services::ShownFrames const shown{ (*screen)->Display() };
  opeu::services::NativeDevices     native;
  opeu::services::Machine           machine{ WithDisplay(native.Devices(), (*screen)->Display()) };
  opeu::game::Game                  game{ state, machine };
  opeu_application_entry(game);
  return {};
}

auto ToPath(std::string_view const text) -> std::filesystem::path {
  return std::filesystem::path{ text };
}

// Where the run looks for the game's data: --data's directory, else the search ($OPEU_DATA_DIR, the host's places, the
// current directory).
auto PlacesOf(DataCommandLine const& options) -> DataPlaces {
  return DataPlaces{ .named = options.data.transform(ToPath), .search = SearchPlaces(Environment("OPEU_DATA_DIR"), opeu::platform::DataDirectories()) };
}

// The remaster the game draws with: --remaster's, else $OPEU_REMASTER's; nothing without either.
auto RemasterOf(DataCommandLine const& options) -> std::optional<std::filesystem::path> {
  return options.remaster.or_else([] { return Environment("OPEU_REMASTER"); }).transform(ToPath);
}

// --check-data: the verdict on stdout; 0 when the run would start, 3 when it would not.
auto ReportData(DataCheck const& check) -> int {
  std::print("{}", check.text);
  return check.complete ? 0 : NO_GAME_DATA;
}

// No directory holds the game's data: why, on stderr and, in a windowed run, to the player (a message box; in the
// browser the page's file picker).
auto ReportMissingData(opeu::assets::DataReport const& report) -> int {
  auto const text{ std::format("opeu: cannot find the game's data files.\n{}Copy the 26 files of your copy of the game into one of these "
                               "directories, or start opeu with --data DIR (docs/building.md, \"The game's data\").\n",
                               report.Text()) };
  std::print(stderr, "{}", text);
  if (port_options.headless == 0) opeu::platform::ShowMissingData(text);
  return NO_GAME_DATA;
}

#if defined(__APPLE__) || defined(__linux__)
// Diagnostic (OPEU_BACKTRACE): the native call stack on a crash.
auto InstallBacktrace() -> void {
  if (!Environment("OPEU_BACKTRACE")) return;
  std::signal(SIGSEGV, CrashBacktrace);
  std::signal(SIGBUS, CrashBacktrace);
  std::signal(SIGABRT, CrashBacktrace);
}
#else
auto InstallBacktrace() -> void { }
#endif

// The window or the headless surface, the display's start state and the memory arena; false when the platform cannot
// start.
auto StartHost() -> bool {
  PlatConfig const config{
    .title      = "Operation Europe",
    .width      = 640,
    .height     = 480,
    .scale      = port_options.scale,
    .headless   = port_options.headless,
    .fullscreen = 0,
  };
  if (port_options.save_dir != nullptr) plat_set_save_dir(port_options.save_dir);
  if (plat_init(&config) != 0) return false;
  std::atexit(ShutdownPlatform);
  vgalib_init();
  port_memory_init();
  return true;
}

// --dump-arena: the initial arena written to `path`; the arena is the recovered link image and needs no data.
auto DumpOnly(std::filesystem::path const& path) -> int {
  if (!StartHost()) return 1;
  DumpArena(path);
  return 0;
}

// What the launcher ran, in order, over the game's data: the FM driver, the opening, then the game.
auto Play(opeu::intro::IntroRequest const intro, opeu::assets::GameData const& data, std::optional<std::filesystem::path> const& remaster) -> int {
  if (!StartHost()) return 1;
  opeu::services::FilesUseAssets([&data](std::string_view const name) { return data.DosFile(name); }); // data outlives the run: Main owns it
  // The FM driver was resident before the game started.
  port_fm_init();
  // The launcher ran the opening between the FM driver and the game.
  opeu::intro::RunIntro({ .request = intro, .bank_dump = BankDumpPath() });
  if (auto const ran{ RunGame(data, remaster) }; !ran) {
    std::println(stderr, "opeu: cannot open the game's screen: {}", ran.error());
    return 1;
  }
  port_exit(0);
  return 0;
}
} // namespace

auto Main(std::span<std::string_view const> const command_line) -> int {
  auto const intro{ opeu::intro::IntroOptions(command_line) }; // --no-intro / --intro
  auto const asked{ DataOptions(intro.remaining) };            // --data / --check-data / --remaster
  auto const options{ ParseOptions(asked.remaining) };
  if (!options) {
    Usage(command_line.front());
    return 2;
  }
  InstallBacktrace();
  if (options->dump_path) return DumpOnly(ToPath(*options->dump_path));
  if (asked.check_data) return ReportData(CheckData(PlacesOf(asked), EmbeddedData()));
  auto const data{ ChooseData(PlacesOf(asked), EmbeddedData()) }; // the run's one copy of the game's data
  if (!data) return ReportMissingData(data.error());
  return Play(intro.request, *data, RemasterOf(asked));
}
} // namespace opeu::app::detail::entry

// main.cpp of the application module calls this.
auto opeu_main(int argc, char** argv) -> int {
  auto const arguments{
    std::span{ argv, static_cast<std::size_t>(argc) }
    | std::views::transform([](char const* argument) { return std::string_view{ argument }; }
    ) | std::ranges::to<std::vector>()
  };
  return opeu::app::detail::entry::Main(arguments);
}
