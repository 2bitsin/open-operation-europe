// The initial values of the music state's DGROUP fields: no track and no effect loaded (FFh), the bank name and path
// buffers empty.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::intro::detail::music_data {

inline constexpr std::uint8_t         MUSIC_CURRENT{ 0xff };
inline constexpr std::uint8_t         EFFECT_CURRENT{ 0xff };
inline constexpr std::array<char, 26> MUSIC_BANK{};
inline constexpr std::array<char, 14> MUSIC_PATH{};

} // namespace opeu::intro::detail::music_data
