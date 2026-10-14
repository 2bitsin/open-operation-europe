#pragma once
// The FM sound card the KOEI driver (FMDRV, fmdriver.hpp) ran on, natively: the OPL2 chip (opl.hpp, ymfm), the PIT
// that clocked the driver, the memory it read the banks from, the audio output and the --wav and --opl-log
// recordings. fm.cpp defines it and documents the clock; until services::Host owns it, fm.cpp keeps the process's one
// behind the INT 66h functions (docs/platform-surface.md section 6).

#include "fmdriver.hpp"
#include "opl.hpp"
#include "wave-file.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <span>
#include <vector>

namespace opeu::services::detail::fm_card {

inline constexpr auto CHUNK_FRAMES{ 512uz }; // frames rendered at a time

class FmCard final : public FmHost {
public:
  // Installs the driver (FMDRV ran from OPEU.BAT before MAIN), opens the recordings and the audio device.
  auto Init() -> void;
  // Called by every event pump: the driver's time up to the host's (or the stalled audio device's).
  auto Clock() -> void;
  // Finishes the recordings.
  auto Shutdown() -> void;
  // One INT 66h call; NO_DRIVER while the driver is not installed.
  [[nodiscard]] auto Interrupt(Request const& request) -> Reply;
  // The output frames the driver clock has advanced.
  [[nodiscard]] auto Frames() const noexcept -> std::uint64_t {
    return _clock_frames;
  }
  // Whether the caller busy-waits on this call's answer (status polls, a running countdown), so the pump must run.
  [[nodiscard]] static auto Waits(Request const& request, Reply const& reply) noexcept -> bool;

  auto               WriteRegister(std::uint8_t reg, std::uint8_t value) -> void override;
  [[nodiscard]] auto ReadStatus() -> std::uint8_t override;
  [[nodiscard]] auto Peek(FarAddress at) -> std::uint8_t override;
  [[nodiscard]] auto EndOfSong(FarAddress callback, SongQueue queue) -> SongQueue override;

private:
  auto OpenRecordings() -> void;
  auto OpenDevice() -> void;
  auto RenderFrames(std::span<std::int16_t> out, std::size_t frames) -> void;
  auto Advance(std::span<std::int16_t> out, std::size_t frames) -> void;
  auto ClockStalledDevice() -> void;

  Opl2                         _chip;
  FmDriver                     _driver{ *this };
  bool                         _installed{};
  bool                         _audio_device{};  // the audio callback clocks the driver
  std::uint32_t                _tick_phase{};    // PIT input clock units since the last tick
  std::uint64_t                _clock_frames{};  // output frames the driver clock has advanced
  std::uint64_t                _last_clock_us{}; // host time of the last callback / pump clocking
  std::optional<WaveFile>      _wav;
  std::optional<std::ofstream> _opl_log;
  std::vector<std::int16_t>    _scratch = std::vector<std::int16_t>(2 * CHUNK_FRAMES);
};

}

namespace opeu::services {
using detail::fm_card::FmCard;
}
