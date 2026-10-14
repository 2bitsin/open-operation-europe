// SYSLIB non-drawing routines (syslib.asm 3FCE..409A). The drawing half is syslib-gfx.cpp.
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstdint>
#include <ranges>

namespace {

// The registers of the bitwise root: the remainder CX, the argument bits still to bring down DX, the trial root BX.
struct RootRound {
  std::uint32_t remainder;
  std::uint32_t rest;
  std::uint32_t trial;
};

// One round: bring down two bits, try root * 2 + 1; on a borrow the remainder is restored and the root bit stays 0.
[[nodiscard]] auto NextRound(RootRound const round, [[maybe_unused]] int const index) noexcept -> RootRound {
  auto const remainder{ ((round.remainder << 2) | (round.rest >> 14)) & 0xFFFF };
  auto const trial{ ((round.trial << 1) | 1) & 0xFFFF };
  auto const fits{ remainder >= trial };
  auto const root_bit{ fits ? 2u : 0u };
  return { .remainder = fits ? remainder - trial : remainder, .rest = (round.rest << 2) & 0xFFFF, .trial = (trial & 0xFF00) | ((trial | root_bit) & 0xFE) };
}

}

// 3FCE isqrt: the argument is the 16-bit word at [bp+6]. Bit 15 set returns FFFFh. Otherwise eight rounds of the
// bitwise root on CX:DX with the trial value in BX; the result is (BX >> 1) & 0FFh (AH cleared).
auto opeu_isqrt(uint32_t const value) -> uint16_t {
  auto const word{ value & 0xFFFF };
  if ((word & 0x8000) != 0) return 0xFFFF;
  auto const last{ std::ranges::fold_left(std::views::iota(0, 8), RootRound{ .remainder = 0, .rest = word, .trial = 0 }, NextRound) };
  return static_cast<std::uint16_t>((last.trial >> 1) & 0xFF);
}

// 402E: INT 21h 2523h/2524h. Ctrl-Break and critical errors have no counterpart on the host (file errors return DOS
// error codes instead).
auto koei_install_dos_handlers() -> void { }

// 4048: loads ES and restores it; no effect.
auto opeu_fm_noop() -> void { }

// 4052: INT 66h AX=F101h, BX=ticks.
auto opeu_fm_wait_start(uint16_t const ticks) -> void {
  port_fm_function(0xF1, 0x01, ticks, 0, 0);
}

// 405F: INT 66h AX=F100h; returns BX (xchg ax,bx).
auto opeu_fm_busy() -> uint16_t {
  return port_fm_function(0xF1, 0x00, 0, 0, 0);
}

// 4066: INT 16h AH=01h; 1 when a key is waiting.
auto opeu_turn_input_b() -> uint16_t {
  return port_key_available() != 0 ? 1 : 0;
}

// 4074: AH=00h with DS:DX = driver data; if AX=0, AH=10h with DS:DX = effect data. Returns 1 when both calls returned
// 0 (the driver is open), 0 otherwise (the original's AX + 1 with AX 0 or FFFFh).
auto opeu_sound_open(uint16_t const driver_off, uint16_t const driver_seg, uint16_t const effects_off, uint16_t const effects_seg) -> int16_t {
  auto const opened{ port_fm_function(0x00, 0, 0, driver_off, driver_seg) == 0 && port_fm_function(0x10, 0, 0, effects_off, effects_seg) == 0 };
  return opened ? 1 : 0;
}
