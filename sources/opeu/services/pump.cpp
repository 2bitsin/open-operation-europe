// Host event pump, presentation pacing and the services' time base.
//
// Every polling service (key available/read, mouse state, FM countdown and status) calls port_pump(). The game never
// waits on anything else, so this is where host events enter the keyboard/mouse driver state and where the VRAM model
// is shown (at most about 60 frames per second).
//
// Window mode: host time; when the pump runs many times in a row without a host event it sleeps 1 ms now and then, so
// busy-waits do not take a whole CPU. The window close request leaves the game (port_exit(0)).
//
// Headless mode (--headless): no window; every pump advances the virtual clock by 1 ms. Input comes from the script
// given with --input; when the virtual clock reaches --ms the screen is rendered, written to --png and the program
// exits with 0.
//
// Input script format (one event per line, '#' starts a comment):
//
//   <ms> key <name>              press and release a key
//   <ms> key <scan>:<ascii>      the same with BIOS codes in hex (1c:0d)
//   <ms> text <characters>       one key per character (letters, digits, space and the US punctuation)
//   <ms> move <x> <y>            move the pointer (screen pixels, 0..639 x 0..479; the game band starts at y=40)
//   <ms> click <x> <y> [left|right]  move there, press the button at <ms> and release it 100 ms later
//   <ms> press <x> <y> [left|right] / <ms> release <x> <y> [left|right]
//
// <ms> is the virtual time in milliseconds. Key names: enter, esc, space, tab, bs, up, down, left, right, home, end,
// pgup, pgdn, ins, del, f1..f10, a..z, 0..9 (top row), kp0..kp9 (keypad, Num Lock off: scan code only, as the cursor
// keys; textout maps scan codes 47h..52h to digits).
#include "hostinput.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <format>
#include <fstream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#endif

namespace opeu::services::detail::pump {
namespace {

// ---- scripted input (headless) ----

// A BIOS key: AH=scan code, AL=ASCII.
struct Key {
  std::uint8_t scan{};
  std::uint8_t ascii{};
};

enum class Action : std::uint8_t {
  KEY,
  MOVE,
  PRESS,
  RELEASE,
};

struct ScriptEvent {
  std::uint32_t ms{};
  Action        action{};
  Key           key{};
  int           x{};
  int           y{};
  int           button{};
};

struct KeyName {
  std::string_view name;
  Key              key;
};

constexpr std::array<KeyName, 35> KEY_NAMES{
  {
   { "enter", { 0x1C, 0x0D } }, { "esc", { 0x01, 0x1B } }, { "space", { 0x39, 0x20 } }, { "tab", { 0x0F, 0x09 } }, { "bs", { 0x0E, 0x08 } }, { "up", { 0x48, 0 } },   { "down", { 0x50, 0 } },
   { "left", { 0x4B, 0 } },     { "right", { 0x4D, 0 } },  { "home", { 0x47, 0 } },     { "end", { 0x4F, 0 } },    { "pgup", { 0x49, 0 } },  { "pgdn", { 0x51, 0 } }, { "ins", { 0x52, 0 } },
   { "del", { 0x53, 0 } },      { "f1", { 0x3B, 0 } },     { "f2", { 0x3C, 0 } },       { "f3", { 0x3D, 0 } },     { "f4", { 0x3E, 0 } },    { "f5", { 0x3F, 0 } },   { "f6", { 0x40, 0 } },
   { "f7", { 0x41, 0 } },       { "f8", { 0x42, 0 } },     { "f9", { 0x43, 0 } },       { "f10", { 0x44, 0 } },    { "kp0", { 0x52, 0 } },   { "kp1", { 0x4F, 0 } },  { "kp2", { 0x50, 0 } },
   { "kp3", { 0x51, 0 } },      { "kp4", { 0x4B, 0 } },    { "kp5", { 0x4C, 0 } },      { "kp6", { 0x4D, 0 } },    { "kp7", { 0x47, 0 } },   { "kp8", { 0x48, 0 } },  { "kp9", { 0x49, 0 } },
   }
};

// US layout: scan codes of the printable characters (unshifted, shifted).
constexpr std::string_view             ROW_CHARS{ "1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./" };
constexpr std::string_view             ROW_SHIFT{ "!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>?" };
constexpr std::array<std::uint8_t, 47> ROW_SCAN{
  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
  0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
};

// The key that types a character.
auto CharKey(char const c) -> std::optional<Key> {
  if (c == ' ') return Key{ .scan = 0x39, .ascii = ' ' };
  auto const row{ ROW_CHARS.find(c) != std::string_view::npos ? ROW_CHARS.find(c) : ROW_SHIFT.find(c) };
  if (c == '\0' || row == std::string_view::npos) return std::nullopt;
  return Key{ .scan = ROW_SCAN.at(row), .ascii = static_cast<std::uint8_t>(c) };
}

// A hex number filling the whole text.
auto Hex(std::string_view const text) -> std::optional<std::uint32_t> {
  std::uint32_t value{};
  auto const [end, error]{ std::from_chars(text.data(), text.data() + text.size(), value, 16) };
  if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
  return value;
}

// <scan>:<ascii> in hex.
auto CodeKey(std::string_view const word) -> std::optional<Key> {
  auto const colon{ word.find(':') };
  if (colon == std::string_view::npos) return std::nullopt;
  auto const scan{ Hex(word.substr(0, colon)) };
  auto const ascii{ Hex(word.substr(colon + 1)) };
  if (!scan || !ascii) return std::nullopt;
  return Key{ .scan = static_cast<std::uint8_t>(*scan), .ascii = static_cast<std::uint8_t>(*ascii) };
}

auto ParseKey(std::string_view const word) -> std::optional<Key> {
  auto const named{ std::ranges::find(KEY_NAMES, word, &KeyName::name) };
  if (named != KEY_NAMES.end()) return named->key;
  if (auto const code{ CodeKey(word) }) return code;
  if (word.size() == 1) return CharKey(word.front());
  return std::nullopt;
}

auto WithAction(ScriptEvent event, Action const action) -> ScriptEvent {
  event.action = action;
  return event;
}

// Reads an input script; a malformed line ends the program with the file and line named.
class ScriptReader {
public:
  explicit ScriptReader(std::string path) : _path{ std::move(path) } { }

  [[nodiscard]] auto Events() -> std::vector<ScriptEvent> {
    std::ifstream file{ _path };
    if (!file) port_fatal("cannot open input script %s", _path.c_str());
    std::vector<ScriptEvent> events;
    std::string              line;
    // A hand loop: one pass per line of the file.
    while (std::getline(file, line)) {
      ++_number;
      std::ranges::copy(Parse(line.substr(0, line.find('#'))), std::back_inserter(events));
    }
    std::ranges::stable_sort(events, {}, &ScriptEvent::ms); // equal times keep the script's order
    return events;
  }

private:
  [[noreturn]] auto Fail(std::string_view const message) const -> void {
    port_fatal("%s", std::format("{}:{}: {}", _path, _number, message).c_str());
    std::unreachable(); // port_fatal exits
  }

  [[nodiscard]] auto Parse(std::string const& line) const -> std::vector<ScriptEvent> {
    std::istringstream arguments{ line };
    std::uint64_t      ms{};
    std::string        verb;
    if (!(arguments >> ms >> verb)) return {};
    auto const at{ ScriptEvent{ .ms = static_cast<std::uint32_t>(ms) } };
    if (verb == "key") return { KeyEvent(at, arguments) };
    if (verb == "text") return Text(at, arguments.eof() ? std::string_view{} : std::string_view{ line }.substr(static_cast<std::size_t>(arguments.tellg())));
    if (verb == "move" || verb == "click" || verb == "press" || verb == "release") return Pointer(at, verb, arguments);
    Fail(std::format("unknown event '{}'", verb));
  }

  [[nodiscard]] auto KeyEvent(ScriptEvent const& at, std::istringstream& arguments) const -> ScriptEvent {
    std::string word;
    arguments >> word;
    auto const key{ ParseKey(word) };
    if (word.empty() || !key) Fail("unknown key");
    return { .ms = at.ms, .action = Action::KEY, .key = *key };
  }

  // `text <characters>`: one key per character up to the end of the line.
  [[nodiscard]] auto Text(ScriptEvent const& at, std::string_view const rest) const -> std::vector<ScriptEvent> {
    auto const start{ std::min(rest.find_first_not_of(" \t"), rest.size()) };
    auto const text{ rest.substr(start, rest.substr(start).find_first_of("\r\n")) };
    return text | std::views::transform([&](char const c) {
             auto const key{ CharKey(c) };
             if (!key) Fail(std::format("no key for '{}'", c));
             return ScriptEvent{ .ms = at.ms, .action = Action::KEY, .key = *key };
           })
           | std::ranges::to<std::vector>();
  }

  // `move|click|press|release <x> <y> [left|right]`; a click is a press and the release 100 ms later.
  [[nodiscard]] auto Pointer(ScriptEvent at, std::string const& verb, std::istringstream& arguments) const -> std::vector<ScriptEvent> {
    std::string button;
    if (!(arguments >> at.x >> at.y)) Fail("expected <x> <y>");
    arguments >> button;
    at.button = button == "right" ? 1 : 0;
    if (verb == "move") return { WithAction(at, Action::MOVE) };
    if (verb == "press") return { WithAction(at, Action::PRESS) };
    if (verb == "release") return { WithAction(at, Action::RELEASE) };
    auto release{ WithAction(at, Action::RELEASE) };
    release.ms += 100;
    return { WithAction(at, Action::PRESS), release };
  }

  std::string _path;
  int         _number{};
};

auto Dispatch(ScriptEvent const& event) -> void {
  switch (event.action) {
  case Action::KEY:     KeyPressed(event.key.scan, event.key.ascii); break;
  case Action::MOVE:    MouseMoved(event.x, event.y); break;
  case Action::PRESS:   MouseButton({ .button = event.button, .down = true, .x = event.x, .y = event.y }); break;
  case Action::RELEASE: MouseButton({ .button = event.button, .down = false, .x = event.x, .y = event.y }); break;
  }
}

// The scripted events, run when their time has come.
class InputScript {
public:
  InputScript() = default;
  explicit InputScript(std::vector<ScriptEvent> events) : _events{ std::move(events) } { }

  auto Run(std::uint32_t const now_ms) -> void {
    auto const pending{ std::span{ _events }.subspan(_next) };
    auto const due{ static_cast<std::size_t>(std::ranges::find_if(pending, [now_ms](ScriptEvent const& event) { return event.ms > now_ms; }) - pending.begin()) };
    _next += due;
    std::ranges::for_each(pending.first(due), [now_ms](ScriptEvent const& event) {
      port_trace("input", "%u ms: script event %d", now_ms, static_cast<int>(event.action));
      Dispatch(event);
    });
  }

private:
  std::vector<ScriptEvent> _events;
  std::size_t              _next{};
};

// Diagnostic: where the game was waiting when a headless run stopped (OPEU_BACKTRACE set).
auto PrintBacktrace() -> void {
#if defined(__APPLE__) || defined(__linux__)
  if (std::getenv("OPEU_BACKTRACE") == nullptr) return;
  std::array<void*, 64> frames{};
  auto const            count{ backtrace(frames.data(), static_cast<int>(frames.size())) };
  backtrace_symbols_fd(frames.data(), count, 2);
#endif
}

// ---- the pump ----

class Pump {
public:
  [[nodiscard]] auto TimeUs() const -> std::uint64_t {
    return port_options.headless != 0 ? _virtual_us : platform::TicksUs();
  }

  auto Run() -> void {
    if (port_options.headless != 0) {
      HeadlessRun();
    } else {
      WindowRun();
    }
  }

private:
  // Renders the screen; true when it differs from what was shown.
  [[nodiscard]] auto Render() -> bool {
    auto const changed{ VgalibRender(std::span{ _frame }.first<hostinput::FRAME_PIXELS>(), _palette) };
    auto const pointer{ PointerDirty() };
    return changed || pointer || !_presented_once;
  }

  auto Show() -> void {
    DrawPointer(std::span{ _frame }.first<hostinput::FRAME_PIXELS>(), _palette);
    platform::Present({ .pixels = _frame, .width = 640, .height = 480, .palette = _palette });
    _presented_once = true;
  }

  auto Present() -> void {
    static_cast<void>(Render());
    Show();
  }

  auto PresentChanges() -> void {
    if (Render()) Show();
  }

  auto LoadScript() -> void {
    if (_script) return;
    _script = port_options.input_path != nullptr ? InputScript{ ScriptReader{ port_options.input_path }.Events() } : InputScript{};
  }

  auto Shoot(std::uint32_t const now_ms) -> void {
    auto const path{ std::format("{}/{:08}.png", port_options.shot_dir, now_ms) };
    Present();
    if (!platform::WritePng(path)) port_fatal("cannot write %s", path.c_str());
  }

  auto Stop(std::uint32_t const now_ms) -> void {
    Present();
    if (port_options.png_path != nullptr && !platform::WritePng(port_options.png_path)) port_fatal("cannot write %s", port_options.png_path);
    port_trace("svc", "headless stop at %u ms", now_ms);
    PrintBacktrace();
    port_exit(0);
  }

  auto HeadlessRun() -> void {
    LoadScript();
    _virtual_us += 1000;
    port_fm_clock();
    auto const now_ms{ static_cast<std::uint32_t>(_virtual_us / 1000) };
    _script->Run(now_ms);
    if (port_options.shot_dir != nullptr && port_options.shot_every_ms != 0 && now_ms % port_options.shot_every_ms == 0) Shoot(now_ms);
    if (port_options.stop_ms != 0 && now_ms >= port_options.stop_ms) Stop(now_ms);
  }

  // The host's events into the devices; true when there were any.
  static auto PollHost() -> bool {
    auto any{ false };
    // A hand loop: the host's queue is drained; its length is not known before.
    while (auto const event{ platform::PollEvent() }) {
      using enum platform::EventType;
      any = true;
      switch (event->type) {
      case QUIT:       port_exit(0); break;
      case KEY_DOWN:   KeyPressed(event->scancode, event->ascii); break;
      case MOUSE_MOVE: MouseMoved(event->x, event->y); break;
      case MOUSE_DOWN:
      case MOUSE_UP:   MouseButton({ .button = event->button, .down = event->type == MOUSE_DOWN, .x = event->x, .y = event->y }); break;
      default:         break;
      }
    }
    return any;
  }

  // Every 64th idle pump sleeps 1 ms.
  auto Idle() -> void {
    if (++_idle % 64u == 0) platform::SleepMs(1);
  }

  auto WindowRun() -> void {
    auto const now{ platform::TicksUs() };
    if (now - _last_poll_us < 500u) {
      Idle();
      return;
    }
    _last_poll_us = now;
    port_fm_clock();
    auto const any{ PollHost() };
    // --input in a window: the script's times are host milliseconds since start (unattended windowed runs, e.g. to
    // hear the music)
    if (port_options.input_path != nullptr) {
      LoadScript();
      _script->Run(static_cast<std::uint32_t>(platform::TicksUs() / 1000));
    }
    auto const later{ platform::TicksUs() };
    if (later - _last_present_us >= 16667u) {
      _last_present_us = later;
      PresentChanges();
    }
    if (any) {
      _idle = 0;
    } else {
      Idle();
    }
  }

  std::vector<std::uint8_t>                          _frame = std::vector<std::uint8_t>(hostinput::FRAME_PIXELS);
  std::array<std::uint8_t, hostinput::PALETTE_BYTES> _palette{};
  std::uint64_t                                      _virtual_us{};
  std::uint64_t                                      _last_present_us{};
  bool                                               _presented_once{};
  std::optional<InputScript>                         _script;
  std::uint32_t                                      _idle{};
  std::uint64_t                                      _last_poll_us{};
};

// The pump of the process (legacy: port_pump and port_time_us are free functions).
auto ThePump() -> Pump& {
  static Pump pump;
  return pump;
}

}
}

auto port_time_us() -> std::uint64_t {
  return opeu::services::detail::pump::ThePump().TimeUs();
}

void port_pump() {
  opeu::services::detail::pump::ThePump().Run();
}
