#pragma once
// The FM banks as the recovered programs hold them (game/data/fmusic.hpp, intro/recovered/data/song-bank.hpp), in the
// order of assets::MUSIC_TABLES, and the register stream the FM driver writes playing every entry of a bank: the
// check that a music table gives the driver what the program gave it.

#include "opeu/assets/music-bank.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace opeu::probes::extract_assets::detail::music_banks {

// MAIN's song bank, MAIN's effect bank, the opening's song bank.
[[nodiscard]] auto ShippedBanks() -> std::array<std::vector<std::byte>, 3>;

// Every register write (register << 8 | value) of the FM driver loading the bank and playing each entry in turn, a
// song for 30 seconds of driver ticks with its repeat on, an effect for 2 seconds.
[[nodiscard]] auto RegisterStream(std::span<std::byte const> bank, assets::BankLayout layout) -> std::vector<std::uint32_t>;

}

namespace opeu::probes::extract_assets {
using detail::music_banks::RegisterStream;
using detail::music_banks::ShippedBanks;
}
