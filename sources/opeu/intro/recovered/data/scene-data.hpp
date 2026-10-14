// The initial values of the scenes' DGROUP fields: the frame tables of the units scene, the two bobbing curves of the
// bomber, the explosion, flak and shell tables, the muzzle flash state, and the strings the opening passes to the
// allocation-failure report and the text console.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::intro::detail::scene_data {

// Reserved bytes (an expanded-memory device name and a zero word) the opening never reads; they hold DGROUP's layout.
inline constexpr std::array<char, 8> RESERVED_EMS_NAME{ 'E', 'M', 'M', 'X', 'X', 'X', 'X', '0' };
inline constexpr std::int16_t        RESERVED_SPRITE_MAPPED{ 0 };

inline constexpr std::int16_t ABORTED{ 0 }; // whether input aborted a scene

// The units scene's frames: rows, heights and capture-buffer offsets.
inline constexpr std::array<std::int16_t, 11>  FRAME_Y{ 196, 196, 100, 100, 100, 100, 100, 164, 228, 196, 196 };
inline constexpr std::array<std::int16_t, 11>  FRAME_H{ 96, 96, 192, 192, 192, 192, 192, 128, 64, 96, 96 };
inline constexpr std::array<std::uint16_t, 11> FRAME_OFF{ 0x0000, 0x1e00, 0x3c00, 0x7800, 0xb400, 0x0000, 0x3c00, 0x7800, 0xa000, 0xb400, 0xd200 };

// The bobbing curve of the bomber as it flies in (pixel rows).
inline constexpr std::array<std::uint16_t, 20> BOB_A{ 80, 83, 85, 87, 90, 90, 87, 85, 83, 80, 80, 77, 75, 73, 70, 70, 73, 75, 77, 80 };

// The explosions (palette steps per frame, palette rows, positions, mask and sprite offsets in the far buffers) and
// the flak flashes' palette rows.
inline constexpr std::array<std::int16_t, 6>  BLAST_STEPS{ 1, 1, 2, 3, 4, 4 };
inline constexpr std::array<std::uint8_t, 3>  BLAST_SLOTS{ 0x27, 0x28, 0x29 };
inline constexpr std::array<std::int16_t, 3>  BLAST_X{ 32, 29, 15 };
inline constexpr std::array<std::int16_t, 3>  BLAST_Y{ 64, 168, 104 };
inline constexpr std::array<std::uint16_t, 6> BLAST_MASKS{ 0x0000, 0x11e0, 0x23c0, 0x35a0, 0x4780, 0x5960 };
inline constexpr std::array<std::uint16_t, 6> BLAST_SPRITES{ 0x0000, 0x4780, 0x8f00, 0x0000, 0x4780, 0x8f00 };
inline constexpr std::array<std::uint8_t, 3>  FLASH_SLOTS{ 0x2b, 0x2c, 0x2d };

// The bobbing curve of the bomber among the flak.
inline constexpr std::array<std::uint16_t, 20> BOB_B{ 80, 83, 85, 87, 90, 90, 87, 85, 83, 80, 80, 77, 75, 73, 70, 70, 73, 75, 77, 80 };

// The flak bursts, the gun crew's flips per salvo and the six shell frames (position, width in columns for drawing
// and saving, height, sprite and save offsets).
inline constexpr std::array<std::int16_t, 4>  FLAK_X{ 14, 68, 40, 26 };
inline constexpr std::array<std::int16_t, 4>  FLAK_Y{ 80, 160, 30, 250 };
inline constexpr std::array<std::int16_t, 3>  GUN_FLIPS{ 10, 20, 6 };
inline constexpr std::array<std::int16_t, 6>  SHELL_X{ 72, 57, 50, 50, 49, 49 };
inline constexpr std::array<std::int16_t, 6>  SHELL_Y{ 208, 264, 280, 288, 296, 296 };
inline constexpr std::array<std::int16_t, 6>  SHELL_W{ 8, 6, 4, 1, 2, 1 };
inline constexpr std::array<std::int16_t, 6>  SHELL_WS{ 8, 6, 4, 2, 2, 2 };
inline constexpr std::array<std::int16_t, 6>  SHELL_H{ 40, 16, 16, 8, 8, 8 };
inline constexpr std::array<std::uint16_t, 6> SHELL_SPRITE{ 0x6b40, 0x6c80, 0x6ce0, 0x6d20, 0x6d28, 0x6d38 };
inline constexpr std::array<std::uint16_t, 6> SHELL_SAVE{ 0x0000, 0x0500, 0x0680, 0x0780, 0x07c0, 0x0800 };

inline constexpr std::int16_t FLASH_ON{ 0 };
inline constexpr std::int16_t FLASH_PHASE{ 0 };

// The allocation-failure tags (there is no alloc_err8), the console reset sequence (ESC R blank: left 0; ESC C 7:
// ink 7; ESC M 0: paper 0; ESC B 0: background 0) and the scenario selection file the shutdown waits for.
inline constexpr std::array<char, 11> ALLOC_ERR1{ std::to_array("alloc_err1") };
inline constexpr std::array<char, 11> ALLOC_ERR2{ std::to_array("alloc_err2") };
inline constexpr std::array<char, 11> ALLOC_ERR3{ std::to_array("alloc_err3") };
inline constexpr std::array<char, 11> ALLOC_ERR4{ std::to_array("alloc_err4") };
inline constexpr std::array<char, 11> ALLOC_ERR5{ std::to_array("alloc_err5") };
inline constexpr std::array<char, 11> ALLOC_ERR6{ std::to_array("alloc_err6") };
inline constexpr std::array<char, 11> ALLOC_ERR7{ std::to_array("alloc_err7") };
inline constexpr std::array<char, 11> ALLOC_ERR9{ std::to_array("alloc_err9") };
inline constexpr std::array<char, 13> CONSOLE_RESET{ std::to_array("\033R \033C7\033M0\033B0") };
inline constexpr std::array<char, 11> SCENARIO_FILE_NAME{ std::to_array("SCESEL.DAT") };

} // namespace opeu::intro::detail::scene_data
