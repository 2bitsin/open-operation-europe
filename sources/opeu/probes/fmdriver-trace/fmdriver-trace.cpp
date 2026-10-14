// Trace runner for the native FM driver (services/fmdriver.cpp).
//
// Reads a command script on stdin and prints every OPL register write and every call result on stdout. Script lines
// (numbers in hex):
//
//   machine <opl_present>        fresh banks and driver; chip present or not
//   mem <seg> <off> <hexbytes>   bank bytes at seg:off (a 64 KiB block per segment, at most four; elsewhere reads 0)
//   install                      install with chip detection
//   uninstall                    INT 66h AH=FFh (the installer's OFF)
//   int <ax> <bx> <dx> <ds>      INT 66h call  -> "ret ax bx dx ds"
//   far <ax> <bx> <dx> <ds>      far-call entry -> "ret ax bx dx ds"
//   tick <n>                     n PIT ticks, each printed as "tick"
//   callbacks                    -> "callbacks <n>" (client callback count)
//   none                         no driver activity (an installer run that only reports, e.g. "already installed")
//
// Output: "> <command>" when a command starts (one "> tick" per tick), "opl <reg> <value>" per register write, in
// order.
#include "fmdriver-trace.hpp"

#include "fmdriver.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

namespace opeu::probes::detail::fmdriver_trace {
namespace {

using services::FarAddress;
using services::FmDriver;
using services::Request;
using services::SongQueue;

constexpr auto BANK_BYTES{ 0x10000uz };
constexpr auto BANK_LIMIT{ 4uz };

// One 64 KiB block of the caller's memory, named by the segment it starts at.
struct Bank {
  std::uint16_t             segment{};
  std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(BANK_BYTES);

  [[nodiscard]] auto Base() const -> std::uint32_t {
    return std::uint32_t{ segment } * 16;
  }
  [[nodiscard]] auto Holds(std::uint32_t const linear) const -> bool {
    return linear >= Base() && linear - Base() < BANK_BYTES;
  }
};

// The validator's machine: the banks, a chip model for the detection, a callback that only counts.
class TraceHost final : public services::FmHost {
public:
  TraceHost(std::string& out, bool const opl_present) : _out{ out }, _opl_present{ opl_present } { }

  auto WriteRegister(std::uint8_t const reg, std::uint8_t const value) -> void override {
    std::format_to(std::back_inserter(_out.get()), "opl {:02x} {:02x}\n", reg, value);
    // The chip model: timer 1 overflows during the detection delay; an IRQ reset (bit 7) clears the status.
    if (!_opl_present || reg != 4) return;
    if ((value & 0x80) != 0) {
      _status = 0;
    } else if ((value & 1) != 0) {
      _status = 0xC0;
    }
  }

  [[nodiscard]] auto ReadStatus() -> std::uint8_t override {
    return _status;
  }

  // Blocks may overlap (banks 800h paragraphs apart): the nearest one starting at or below the address holds it.
  [[nodiscard]] auto Peek(FarAddress const at) -> std::uint8_t override {
    auto const linear{ (std::uint32_t{ at.segment } * 16) + at.offset };
    auto       holders{ _banks | std::views::filter([linear](Bank const& bank) { return bank.Holds(linear); }) };
    auto const best{ std::ranges::max_element(holders, {}, &Bank::segment) };
    return best == holders.end() ? std::uint8_t{ 0 } : best->bytes[linear - best->Base()];
  }

  [[nodiscard]] auto EndOfSong(FarAddress /*callback*/, SongQueue const queue) -> SongQueue override {
    ++_callbacks;
    return queue;
  }

  // The block of `segment`, allocated on first use; none when four are in use.
  [[nodiscard]] auto BankOf(std::uint16_t const segment) -> std::optional<std::reference_wrapper<Bank>> {
    auto const found{ std::ranges::find(_banks, segment, &Bank::segment) };
    if (found != _banks.end()) return *found;
    if (_banks.size() == BANK_LIMIT) return std::nullopt;
    return _banks.emplace_back(Bank{ .segment = segment });
  }

  [[nodiscard]] auto Callbacks() const -> std::uint32_t {
    return _callbacks;
  }

private:
  std::reference_wrapper<std::string> _out;
  bool                                _opl_present;
  std::uint8_t                        _status{};
  std::uint32_t                       _callbacks{};
  std::vector<Bank>                   _banks;
};

// One line of the script.
struct Line {
  std::string text;
};

auto operator>>(std::istream& in, Line& line) -> std::istream& {
  return std::getline(in, line.text);
}

// A hex number, if the text starts with one.
auto Hex(std::string_view const text) -> std::optional<std::uint32_t> {
  std::uint32_t     value{};
  auto const* const begin{ std::to_address(text.begin()) };
  auto const [end, error]{ std::from_chars(begin, std::to_address(text.end()), value, 16) };
  if (error != std::errc{} || end == begin) return std::nullopt;
  return value;
}

// The hex numbers of a command's arguments.
auto Numbers(std::istringstream& arguments) -> std::vector<std::uint32_t> {
  return std::views::istream<std::string>(arguments) | std::views::transform(Hex) | std::views::take_while([](auto const& n) { return n.has_value(); })
         | std::views::transform([](auto const& n) { return *n; }) | std::ranges::to<std::vector>();
}

// A chip model and the driver that talks to it; the driver keeps a reference to the host, so the pair never moves.
class Machine {
public:
  Machine(std::string& out, bool const opl_present) : _host{ out, opl_present }, _driver{ _host } { }

  [[nodiscard]] auto Host() -> TraceHost& {
    return _host;
  }
  [[nodiscard]] auto Driver() -> FmDriver& {
    return _driver;
  }

private:
  TraceHost _host;
  FmDriver  _driver;
};

class Trace {
public:
  // One script line; false when the script is malformed.
  [[nodiscard]] auto Execute(std::string const& line) -> bool;
  [[nodiscard]] auto Output() const -> std::string const& {
    return _out;
  }

private:
  [[nodiscard]] auto Store(std::istringstream& arguments) -> bool;
  [[nodiscard]] auto Invoke(std::string const& verb, std::istringstream& arguments) -> bool;
  auto               Tick(std::istringstream& arguments) -> void;
  auto               NewMachine(std::istringstream& arguments) -> void;

  std::string              _out; // the trace, written at the end (stdout one line at a time is slow)
  std::unique_ptr<Machine> _machine{ std::make_unique<Machine>(_out, false) };
};

auto Trace::NewMachine(std::istringstream& arguments) -> void {
  auto const numbers{ Numbers(arguments) };
  _machine = std::make_unique<Machine>(_out, !numbers.empty() && numbers.front() != 0);
}

auto Trace::Store(std::istringstream& arguments) -> bool {
  std::string segment;
  std::string offset;
  std::string bytes;
  arguments >> segment >> offset >> bytes;
  auto const at{ Hex(segment) };
  auto const from{ Hex(offset) };
  if (!at || !from || bytes.empty()) return false;
  auto const bank{ _machine->Host().BankOf(static_cast<std::uint16_t>(*at)) };
  if (!bank) return false;
  auto const text{ std::string_view{ bytes } };
  return std::ranges::all_of(std::views::iota(0uz, text.size() / 2), [&](std::size_t const i) {
    auto const value{ Hex(text.substr(2 * i, 2)) };
    if (value) bank->get().bytes[static_cast<std::uint16_t>(*from + i)] = static_cast<std::uint8_t>(*value);
    return value.has_value();
  });
}

auto Trace::Invoke(std::string const& verb, std::istringstream& arguments) -> bool {
  auto const numbers{ Numbers(arguments) };
  if (numbers.size() < 4) return false;
  auto const request{
    Request{ .function = static_cast<std::uint8_t>(numbers[0] >> 8),
            .argument = static_cast<std::uint8_t>(numbers[0]),
            .ticks    = static_cast<std::uint16_t>(numbers[1]),
            .address  = { .segment = static_cast<std::uint16_t>(numbers[3]), .offset = static_cast<std::uint16_t>(numbers[2]) } }
  };
  auto const reply{ verb == "int" ? _machine->Driver().Interrupt(request) : _machine->Driver().FarCall(request) };
  std::format_to(std::back_inserter(_out), "ret {:04x} {:04x} {:04x} {:04x}\n", reply.status, reply.ticks, reply.address.offset, reply.address.segment);
  return true;
}

auto Trace::Tick(std::istringstream& arguments) -> void {
  auto const numbers{ Numbers(arguments) };
  auto const ticks{ numbers.empty() ? 0u : numbers.front() };
  std::ranges::for_each(std::views::iota(0u, ticks), [&](std::uint32_t) {
    _out += "> tick\n";
    _machine->Driver().TimerTick();
  });
}

auto Trace::Execute(std::string const& line) -> bool {
  std::istringstream arguments{ line };
  std::string        verb;
  if (!(arguments >> verb)) return true;
  if (verb != "mem" && verb != "tick") std::format_to(std::back_inserter(_out), "> {}\n", verb);
  if (verb == "mem") return Store(arguments);
  if (verb == "int" || verb == "far") return Invoke(verb, arguments);
  if (verb == "machine") {
    NewMachine(arguments);
  } else if (verb == "install") {
    _machine->Driver().InstallDetecting();
  } else if (verb == "uninstall") {
    _machine->Driver().Uninstall();
  } else if (verb == "tick") {
    Tick(arguments);
  } else if (verb == "callbacks") {
    std::format_to(std::back_inserter(_out), "callbacks {:x}\n", _machine->Host().Callbacks());
  } else if (verb != "none") {
    std::cerr << std::format("fmdriver_trace: unknown command {}\n", verb);
    return false;
  }
  return true;
}

}

auto FmdriverTrace() -> int {
  Trace      trace;
  auto       lines{ std::views::istream<Line>(std::cin) };
  auto const ran{ std::ranges::all_of(lines, [&](Line const& line) { return trace.Execute(line.text); }) };
  std::cout << trace.Output();
  return ran ? 0 : 2;
}

}
