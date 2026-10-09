#pragma once
// A broken contract asserted whole in whichever mode the build chose: the
// kind, a real file and line, the function, then the text -- never a
// wildcard that can swallow all four.

#include "opeu/contract.hpp"

#include <gtest/gtest.h>

#include <format>
#include <regex>
#include <source_location>
#include <string>
#include <string_view>

namespace opeu::detail::contract_test {

// The two answers a test needs of the mode it was built with.
inline constexpr auto CONTRACTS_STOP{ CONTRACT_MODE == ContractMode::STOP };
inline constexpr auto CONTRACTS_HALT{ CONTRACT_MODE == ContractMode::HALT };

// Both regex engines that read it -- gtest's death-test POSIX ERE and
// std::regex's ECMAScript -- treat these the same once escaped.
inline auto Quoted(std::string_view text) -> std::string {
  constexpr auto SPECIAL{ std::string_view{ R"(\^$.[]|()*+?{})" } };

  auto result{ std::string{} };
  for (auto const letter : text) {
    if (SPECIAL.find(letter) != SPECIAL.npos) {
      result.push_back('\\');
    }
    result.push_back(letter);
  }
  return result;
}

// ContractLine()'s shape, with the place left open and the text exact.
inline auto ContractPattern(std::string_view kind, std::string_view text) -> std::string {
  return std::format("{}: [^ ]+:[0-9]+ [^ ].*: {}", kind, Quoted(text));
}

inline auto SaysContract(std::string const& said, std::string_view kind, std::string_view text) -> bool {
  return std::regex_search(said, std::regex{ ContractPattern(kind, text) });
}

// What the halt mode throws, with the report it also prints swallowed so a
// passing test says nothing; empty when nothing was thrown.
template <typename _Broken>
auto ThrownContract(_Broken&& broken) -> std::string {
  auto thrown{ std::string{} };
  testing::internal::CaptureStderr();
  try {
    broken();
  } catch (ContractFailure const& failure) {
    thrown = failure.what();
  }
  static_cast<void>(testing::internal::GetCapturedStderr());
  return thrown;
}

// A broken contract of `kind` in whichever mode the build chose: halt throws,
// a mode that stops for this kind dies, any other carries on past the break,
// so `broken` is compiled (what it names is still used) but never run.
// Failures are reported at the caller's line.
template <bool _Stopping, typename _Broken>
auto ExpectStopped(_Broken&& broken, std::string_view kind, std::string_view text, std::source_location where) -> void {
  auto const trace{ testing::ScopedTrace{ where.file_name(), static_cast<int>(where.line()), "broken contract" } };
  if constexpr (CONTRACTS_HALT) {
    auto const thrown{ ThrownContract(broken) };
    EXPECT_TRUE(SaysContract(thrown, kind, text)) << thrown;
  } else if constexpr (_Stopping) {
    EXPECT_DEATH(broken(), ContractPattern(kind, text));
  }
}

template <typename _Broken>
auto ExpectContractPrecondition(_Broken&& broken, std::string_view text,
                                std::source_location where = std::source_location::current()) -> void {
  ExpectStopped<CONTRACTS_STOP>(broken, "precondition", text, where);
}

template <typename _Broken>
auto ExpectContractPostcondition(_Broken&& broken, std::string_view text,
                                 std::source_location where = std::source_location::current()) -> void {
  ExpectStopped<CONTRACTS_STOP>(broken, "postcondition", text, where);
}

// Unreachable has no continuation in any mode, so only halt survives it.
template <typename _Broken>
auto ExpectContractUnreachable(_Broken&& broken, std::string_view text,
                               std::source_location where = std::source_location::current()) -> void {
  ExpectStopped<true>(broken, "unreachable", text, where);
}

}

namespace opeu {
using detail::contract_test::CONTRACTS_HALT;
using detail::contract_test::CONTRACTS_STOP;
using detail::contract_test::ContractPattern;
using detail::contract_test::ExpectContractPostcondition;
using detail::contract_test::ExpectContractPrecondition;
using detail::contract_test::ExpectContractUnreachable;
using detail::contract_test::SaysContract;
using detail::contract_test::ThrownContract;
}
