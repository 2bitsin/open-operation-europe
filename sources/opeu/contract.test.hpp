#pragma once
// A broken contract asserted whole in whichever mode the build chose: the
// kind, a real file and line, the function, then the text -- never a
// wildcard that can swallow all four.

#include "opeu/contract.hpp"

#include <gtest/gtest.h>

#include <format>
#include <regex>
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

  auto result{ std::string{ } };
  for (auto const letter : text) {
    if (SPECIAL.find(letter) != SPECIAL.npos) {
      result.push_back('\\');
    }
    result.push_back(letter);
  }
  return result;
}

// ContractLine()'s shape, with the place left open and the text exact.
inline auto ContractPattern(std::string_view kind, std::string_view text)
    -> std::string {
  return std::format("{}: [^ ]+:[0-9]+ [^ ].*: {}", kind, Quoted(text));
}

inline auto SaysContract(std::string const& said, std::string_view kind,
                         std::string_view text) -> bool {
  return std::regex_search(said, std::regex{ ContractPattern(kind, text) });
}

// What the halt mode throws, with the report it also prints swallowed so a
// passing test says nothing; empty when nothing was thrown.
template <typename _Broken>
auto ThrownContract(_Broken&& broken) -> std::string {
  auto thrown{ std::string{ } };
  testing::internal::CaptureStderr();
  try {
    broken();
  } catch (ContractFailure const& failure) {
    thrown = failure.what();
  }
  static_cast<void>(testing::internal::GetCapturedStderr());
  return thrown;
}

}

namespace opeu {
using detail::contract_test::CONTRACTS_HALT;
using detail::contract_test::CONTRACTS_STOP;
using detail::contract_test::ContractPattern;
using detail::contract_test::SaysContract;
using detail::contract_test::ThrownContract;
}

// Compiled and unevaluated: the statement still type-checks, and what it
// names is still used, but a mode that carries on past a break never runs it.
#define OPEU_SKIP_CONTRACT(statement, text) \
  do {                                      \
    if (false) {                            \
      statement;                            \
    }                                       \
    static_cast<void>(text);                \
  } while (false)

#define OPEU_EXPECT_THROWN(statement, kind, text)                    \
  do {                                                               \
    auto const thrown{ ::opeu::ThrownContract([&] { statement; }) }; \
    EXPECT_TRUE(::opeu::SaysContract(thrown, kind, text)) << thrown; \
  } while (false)

// halt throws, a mode that stops for this kind dies, any other skips.
#define OPEU_EXPECT_STOPPED(stopping, statement, kind, text)        \
  do {                                                              \
    if constexpr (::opeu::CONTRACTS_HALT) {                         \
      OPEU_EXPECT_THROWN(statement, kind, text);                    \
    } else if constexpr (stopping) {                                \
      EXPECT_DEATH(statement, ::opeu::ContractPattern(kind, text)); \
    } else {                                                        \
      OPEU_SKIP_CONTRACT(statement, text);                          \
    }                                                               \
  } while (false)

#define EXPECT_CONTRACT_PRECONDITION(statement, text) \
  OPEU_EXPECT_STOPPED(::opeu::CONTRACTS_STOP, statement, "precondition", text)
#define EXPECT_CONTRACT_POSTCONDITION(statement, text) \
  OPEU_EXPECT_STOPPED(::opeu::CONTRACTS_STOP, statement, "postcondition", text)
// Unreachable has no continuation in any mode, so only halt survives it.
#define EXPECT_CONTRACT_UNREACHABLE(statement, text) \
  OPEU_EXPECT_STOPPED(true, statement, "unreachable", text)
