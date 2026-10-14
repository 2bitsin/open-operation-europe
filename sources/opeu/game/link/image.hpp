// The conventional memory as the game starts, for the --dump-arena diagnostic (link/image.cpp).
#pragma once

#include <cstdint>
#include <vector>

namespace opeu::game::detail::image_dump {
// The far data the arena holds and, at its old place, the initial data segment of GameState::Initial(). Only
// --dump-arena writes it, for the smoke's comparison; B7 drops both.
auto InitialArenaImage() -> std::vector<std::uint8_t>;
} // namespace opeu::game::detail::image_dump

namespace opeu::game {
using detail::image_dump::InitialArenaImage;
} // namespace opeu::game
