#pragma once
// The six DOS programs of the original game directory, as shipped: what the
// recovery rebuilds and the port replaces. Sizes: docs/STATUS.txt; launch
// order: analysis/koei/launcher.pseudocode.txt.

#include "opeu/contract.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::originals::detail::programs {

using namespace std::string_view_literals;

enum class ProgramFormat : std::uint8_t {
  MZ,   // relocatable EXE: an MZ header, then the load image
  COM,  // flat image loaded at PSP:0100, the whole file
};

[[nodiscard]] auto FormatName(ProgramFormat format) -> std::string_view;

struct Program {
  std::string_view name{ };  // 8.3 name, upper case as shipped
  ProgramFormat    format{ };
  std::size_t      file_bytes{ };
  std::size_t      load_image_bytes{ };  // what DOS copies into memory

  auto operator==(Program const&) const noexcept -> bool = default;

  // e_cparhdr * 16, the bytes in front of the image.
  [[nodiscard]] constexpr auto MzHeaderBytes() const -> std::size_t {
    Expects(format == ProgramFormat::MZ, "a COM program has no MZ header");
    return file_bytes - load_image_bytes;
  }
};

// Sorted by name: ProgramNamed() is a binary search over this order.
//   END.EXE    the ending, run when MAIN.EXE exits with 0
//   FMDRV.COM  resident FM (YM3812) music driver, loaded first
//   KOEI.COM   the launcher: runs FMDRV, OPEN, MAIN, then END
//   KOEI.EXE   the launcher again: KOEI.COM's bytes behind an MZ header
//   MAIN.EXE   the game; exit code 100 quits, 0 runs the ending
//   OPEN.EXE   the opening sequence, with its own music bank
inline constexpr auto PROGRAMS{ std::to_array<Program>({
  { "END.EXE", ProgramFormat::MZ, 41'275uz, 39'739uz },
  { "FMDRV.COM", ProgramFormat::COM, 5'469uz, 5'469uz },
  { "KOEI.COM", ProgramFormat::COM, 678uz, 678uz },
  { "KOEI.EXE", ProgramFormat::MZ, 1'446uz, 934uz },
  { "MAIN.EXE", ProgramFormat::MZ, 365'961uz, 341'385uz },
  { "OPEN.EXE", ProgramFormat::MZ, 46'999uz, 44'951uz },
}) };

// The order KOEI.COM's launch sequence executes them; END.EXE only on 0.
inline constexpr auto LAUNCH_ORDER{ std::array{
  "FMDRV.COM"sv,
  "OPEN.EXE"sv,
  "MAIN.EXE"sv,
  "END.EXE"sv,
} };

[[nodiscard]] auto Programs() noexcept -> std::span<Program const>;

// The program a DOS name names, in any letter case (DOS folds it).
[[nodiscard]] auto ProgramNamed(std::string_view name)
    -> std::optional<Program>;

}

namespace opeu::originals {
using detail::programs::FormatName;
using detail::programs::LAUNCH_ORDER;
using detail::programs::PROGRAMS;
using detail::programs::Program;
using detail::programs::ProgramFormat;
using detail::programs::ProgramNamed;
using detail::programs::Programs;
}
