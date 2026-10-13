// The key codes the game's input loops compare against: what the keyboard service returns for Enter and Escape.
#pragma once

#include <cstdint>

namespace opeu::game::detail::keys {

inline constexpr std::uint8_t KEY_ENTER{ 0x0d };
inline constexpr std::uint8_t KEY_ESCAPE{ 0x1b };
} // namespace opeu::game::detail::keys

namespace opeu::game {
using detail::keys::KEY_ENTER;
using detail::keys::KEY_ESCAPE;
} // namespace opeu::game
