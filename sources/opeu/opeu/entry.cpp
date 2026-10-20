// Operation Europe: the program entry. The host prepares what the DOS start-up and the launcher provided (the
// initialised data segment, the memory arena, the display, input and the FM driver), runs the opening, then calls the
// game's application entry on its machine: the game's screen on SDL_Renderer over the classic pack (game-screen.hpp)
// and the native files, input, sound and clock.
#include "common/game.hpp"
#include "core/app.hpp"
#include "game-screen.hpp"
#include "intro.hpp"
#include "link/image.hpp"
#include "machine.hpp"
#include "native-machine.hpp"
#include "opeu/assets/game-data.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"
#include "shown-frames.hpp"

#include <array>
#include <charconv>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
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

// The game's application entry on its initial state and the machine it runs on: the game's screen, which the pump
// shows from now on, and the native files, input, sound and clock (adapters over the services); it returns after the
// scenario's end. The screen's error when it cannot be opened.
auto RunGame(opeu::assets::GameData const& data) -> std::expected<void, std::string> {
  auto       state{ opeu::game::GameState::Initial() };
  auto const screen{ GameScreen::Open(data, std::nullopt) };
  if (!screen) return std::unexpected{ screen.error() };
  opeu::services::ShownFrames const shown{ (*screen)->Display() };
  opeu::services::NativeDevices     native;
  opeu::services::Machine           machine{ WithDisplay(native.Devices(), (*screen)->Display()) };
  opeu::game::Game                  game{ state, machine };
  opeu_application_entry(game);
  return {};
}
} // namespace

auto Main(std::span<std::string_view const> const command_line, opeu::assets::GameData const& data) -> int {
  auto const intro{ opeu::intro::IntroOptions(command_line) }; // --no-intro / --intro
  auto const options{ ParseOptions(intro.remaining) };
  if (!options) {
    Usage(command_line.front());
    return 2;
  }

#if defined(__APPLE__) || defined(__linux__)
  if (Environment("OPEU_BACKTRACE")) {
    std::signal(SIGSEGV, CrashBacktrace);
    std::signal(SIGBUS, CrashBacktrace);
    std::signal(SIGABRT, CrashBacktrace);
  }
#endif
  PlatConfig const config{
    .title      = "Operation Europe",
    .width      = 640,
    .height     = 480,
    .scale      = port_options.scale,
    .headless   = port_options.headless,
    .fullscreen = 0,
  };
  if (port_options.save_dir != nullptr) plat_set_save_dir(port_options.save_dir);
  if (plat_init(&config) != 0) return 1;
  std::atexit(ShutdownPlatform);

  vgalib_init();
  port_memory_init();
  if (options->dump_path) {
    DumpArena(std::filesystem::path{ *options->dump_path });
    return 0;
  }

  // The FM driver was resident before the game started.
  port_fm_init();
  // The launcher ran the opening between the FM driver and the game.
  opeu::intro::RunIntro({ .request = intro.request, .bank_dump = BankDumpPath() });
  if (auto const ran{ RunGame(data) }; !ran) {
    std::println(stderr, "opeu: cannot open the game's screen: {}", ran.error());
    return 1;
  }
  port_exit(0);
  return 0;
}
} // namespace opeu::app::detail::entry

// main.cpp of the application module calls this.
auto opeu_main(int argc, char** argv, opeu::assets::GameData const& data) -> int {
  auto const arguments{
    std::span{ argv, static_cast<std::size_t>(argc) }
    | std::views::transform([](char const* argument) { return std::string_view{ argument }; }
    ) | std::ranges::to<std::vector>()
  };
  return opeu::app::detail::entry::Main(arguments, data);
}
