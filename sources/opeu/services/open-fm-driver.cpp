// The FM driver calls the opening makes and the game does not: its effect countdown and the opening of its own song
// bank. The routines it shares with the game (the music timeline's start and wait, the driver's no-op, the input
// turn) use the game's services.
//
// The FM driver's two countdowns: counter B is the timeline the opening's scenes wait on; counter A is the one the
// opening's uninterruptible delay uses (these two). Both run at 60.3 Hz in the driver (fmdriver.cpp).
#include "open-services.hpp"

#include "port.hpp"
#include "services.hpp"

#include <cstdint>

namespace opeu::services::detail::open_services {
namespace {
constexpr std::uint8_t COUNTDOWNS{ 0xf0 };   // the driver's countdown function (INT 66h AH=F0h)
constexpr std::uint8_t READ_EFFECT{ 0x00 };  // AL: the effect countdown's ticks left
constexpr std::uint8_t START_EFFECT{ 0x01 }; // AL: start the effect countdown at BX ticks
constexpr std::uint8_t OPEN_BANK{ 0x00 };    // the driver's open with DS:DX = the song bank (INT 66h AH=00h)
} // namespace

auto StartEffectCountdown(std::uint16_t const ticks) -> void {
  port_fm_function(COUNTDOWNS, START_EFFECT, ticks, 0, 0);
}

auto EffectCountdown() -> std::uint16_t {
  return port_fm_function(COUNTDOWNS, READ_EFFECT, 0, 0, 0);
}

// The driver answers 0 when it opened the bank (the game's open without its second call for an effect bank).
auto OpenSongBank(std::uint16_t const offset, std::uint16_t const segment) -> std::int16_t {
  return port_fm_function(OPEN_BANK, 0, 0, offset, segment) == 0 ? 1 : 0;
}
} // namespace opeu::services::detail::open_services
