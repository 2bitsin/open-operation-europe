#pragma once
// Preconditions, postconditions and unreachable branches, bound to the
// buildutil `contracts` option: the option names the mode, a word no mode
// answers to fails the assertion below, and an undeclared one cannot compile.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <optional>
#include <print>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace opeu::detail::contract {

enum class ContractMode : std::uint8_t {
  STOP,     // report on stderr, then std::abort()
  HALT,     // report on stderr, then throw ContractFailure
  COMPLAIN, // report on stderr and carry on
  OFF,      // say nothing; only an unreachable branch still stops
};
// OFF, not IGNORE: <winbase.h> defines IGNORE as a macro.

consteval auto ContractModeNamed(std::string_view name) -> std::optional<ContractMode> {
  if (name == "stop") return ContractMode::STOP;
  if (name == "halt") return ContractMode::HALT;
  if (name == "complain") return ContractMode::COMPLAIN;
  if (name == "ignore") return ContractMode::OFF;
  return std::nullopt;
}

inline constexpr auto NAMED_MODE{ ContractModeNamed(OPEU_CONTRACTS) };
static_assert(NAMED_MODE.has_value(), "contracts is stop, halt, complain or ignore");
inline constexpr auto CONTRACT_MODE{ *NAMED_MODE };

// What the halt mode throws: the same line the report printed.
class ContractFailure : public std::logic_error {
public:
  using std::logic_error::logic_error;
};

// "<kind>: <file>:<line> <function>: <text>", the one shape every mode says.
inline auto ContractLine(std::string_view kind, std::string_view text, std::source_location const& where)
    -> std::string {
  return std::format("{}: {}:{} {}: {}", kind, where.file_name(), where.line(), where.function_name(), text);
}

// The out-of-line half of a broken contract; a held one never reaches it.
[[gnu::cold]] inline auto Broken(std::string_view kind, std::string_view text, std::source_location const& where)
    -> void {
  auto const line{ ContractLine(kind, text, where) };
  std::println(stderr, "{}", line);
  std::fflush(stderr);
  if constexpr (CONTRACT_MODE == ContractMode::STOP) {
    std::abort();
  } else if constexpr (CONTRACT_MODE == ContractMode::HALT) {
    throw ContractFailure{ line };
  }
}

constexpr auto Expects(bool held, std::string_view text, std::source_location where = std::source_location::current())
    -> void {
  if constexpr (CONTRACT_MODE != ContractMode::OFF) {
    if (!held) {
      Broken("precondition", text, where);
    }
  }
}

constexpr auto Ensures(bool held, std::string_view text, std::source_location where = std::source_location::current())
    -> void {
  if constexpr (CONTRACT_MODE != ContractMode::OFF) {
    if (!held) {
      Broken("postcondition", text, where);
    }
  }
}

// A closed switch's default: no mode has a continuation, so every mode but
// halt ends the program here, ignore included. An enumeration is said as
// its number, which is what the original's tables and save files carry.
template <typename _Value>
[[noreturn]] auto Unreachable(_Value const& value, std::source_location where = std::source_location::current())
    -> void {
  auto said{ std::string{} };
  if constexpr (std::is_enum_v<_Value>) {
    said = std::format("{}", std::to_underlying(value));
  } else {
    said = std::format("{}", value);
  }
  auto const line{ ContractLine("unreachable", said, where) };
  std::println(stderr, "{}", line);
  std::fflush(stderr);
  if constexpr (CONTRACT_MODE == ContractMode::HALT) {
    throw ContractFailure{ line };
  }
  std::abort();
}

}

namespace opeu {
using detail::contract::CONTRACT_MODE;
using detail::contract::ContractFailure;
using detail::contract::ContractLine;
using detail::contract::ContractMode;
using detail::contract::Ensures;
using detail::contract::Expects;
using detail::contract::Unreachable;
}
