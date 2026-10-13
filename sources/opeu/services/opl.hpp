// The YM3812 (OPL2) chip as a native audio device: ymfm's emulation of the sound chip (ymfm module, BSD-3-Clause).
// The FM driver (fmdriver.cpp) writes registers; the audio path pulls samples at the chip's own rate (3579545 Hz / 72
// = 49716 Hz, mono).
#pragma once

#include "opeu/ymfm/opl.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <span>

namespace opeu::services::detail::opl {

inline constexpr auto OPL_CLOCK{ 3579545u };
inline constexpr auto OPL_SAMPLE_RATE{ 49716 }; // OPL_CLOCK / 72, rounded

class Opl2 : public ymfm::ymfm_interface {
public:
  Opl2();

  auto Reset() -> void;
  // One register write (address port 388h, then data port 389h).
  auto Write(std::uint8_t reg, std::uint8_t value) -> void;
  // Render interleaved stereo frames (both channels carry the chip's mono output).
  auto Render(std::span<std::int16_t> stereo) -> void;

private:
  [[nodiscard]] auto NextSample() -> std::int16_t;
  auto               RetriggerGap(std::uint32_t struck) -> void;

  ymfm::ym3812                _chip;
  std::array<std::uint8_t, 9> _key_regs{};   // last B0h-B8h values (bit 5: key on)
  std::uint8_t                _rhythm_reg{}; // last BDh value (bits 0-4: drum keys)
  std::uint32_t               _released{};   // keys turned off since the last sample: bit n = channel n, bit 9+d = drum d
  std::deque<std::int16_t>    _queued;       // samples generated ahead by a retrigger
};

}

namespace opeu::services {
using detail::opl::Opl2;
using detail::opl::OPL_CLOCK;
using detail::opl::OPL_SAMPLE_RATE;
}
