// The DOS file services' arguments the game passes: the seek origin.
#pragma once

#include <cstdint>

namespace opeu::game::detail::dos_file {

// The origin of a seek (INT 21h AH=42h, AL): from the start of the file.
inline constexpr std::uint16_t SEEK_FROM_START{ 0 };
} // namespace opeu::game::detail::dos_file

namespace opeu::game {
using detail::dos_file::SEEK_FROM_START;
} // namespace opeu::game
