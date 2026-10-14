// The bomb sprite's 512-byte save buffer, initially zero.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::fg3a99 {

inline constexpr std::array<std::uint8_t, 512> BOMB_SPRITE_SAVE{};

} // namespace opeu::game::detail::fg3a99

namespace opeu::game {
using detail::fg3a99::BOMB_SPRITE_SAVE;
} // namespace opeu::game
