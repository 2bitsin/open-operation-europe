#include "opeu/originals/programs.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <ranges>
#include <string_view>

namespace opeu::originals::detail::programs {

namespace {

// 8 + "." + 3: no DOS name is longer, so a longer one names nothing.
constexpr auto DOS_NAME_BYTES{ 12uz };

constexpr auto UpperCase(char letter) noexcept -> char {
  return letter >= 'a' && letter <= 'z' ? static_cast<char>(letter - 'a' + 'A')
                                        : letter;
}

static_assert(std::ranges::is_sorted(PROGRAMS, std::less{ }, &Program::name),
              "ProgramNamed() searches PROGRAMS by name");

}

auto FormatName(ProgramFormat format) -> std::string_view {
  switch (format) {
    case ProgramFormat::MZ:  return "MZ";
    case ProgramFormat::COM: return "COM";
  }
  Unreachable(format);
}

auto Programs() noexcept -> std::span<Program const> {
  return PROGRAMS;
}

auto ProgramNamed(std::string_view name) -> std::optional<Program> {
  if (name.size() > DOS_NAME_BYTES) {
    return std::nullopt;
  }
  auto folded{ std::array<char, DOS_NAME_BYTES>{ } };
  std::ranges::transform(name, folded.begin(), UpperCase);
  auto const key{ std::string_view{ folded.data(), name.size() } };
  auto const found{ std::ranges::lower_bound(PROGRAMS, key, std::less{ },
                                             &Program::name) };
  if (found == PROGRAMS.end() || found->name != key) {
    return std::nullopt;
  }
  return *found;
}

}
