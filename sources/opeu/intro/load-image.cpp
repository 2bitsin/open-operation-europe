// OPEN's load image: where each recovered data object (recovered/data) lies. Segment 0 is DGROUP, at the offset of the
// object's field in OpenDataSegment; any other segment is relative to OPEN's load segment. OpenMemory::Load copies each
// object there.
#include "load-image.hpp"

#include "recovered.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace opeu::intro::detail::load_image {
namespace {
// The bytes of a recovered data object.
template <class _Object>
auto ObjectBytes(_Object const& object) -> std::span<std::byte const> {
  return std::as_bytes(std::span{ &object, 1 });
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init): only addresses and sizes of the data objects, no values
std::array<OpenImageObject, 71> const IMAGE_OBJECTS{ {
  // The text console
  { ObjectBytes(detail::console_data::UNUSED_DIGIT_FONT),          0x0000,     offsetof(OpenDataSegment, digit_font) },
  { ObjectBytes(detail::console_data::DRAW_HOOKS),                 0x0000,     offsetof(OpenDataSegment, draw_hooks) },
  { ObjectBytes(detail::console_data::TEXT_ERASE),                 0x0000,     offsetof(OpenDataSegment, text_erase) },
  { ObjectBytes(detail::console_data::TEXT_X),                     0x0000,     offsetof(OpenDataSegment, text_x) },
  { ObjectBytes(detail::console_data::TEXT_Y),                     0x0000,     offsetof(OpenDataSegment, text_y) },
  { ObjectBytes(detail::console_data::TEXT_LEFT),                  0x0000,     offsetof(OpenDataSegment, text_left) },
  { ObjectBytes(detail::console_data::TEXT_PAPER),                 0x0000,     offsetof(OpenDataSegment, text_paper) },
  { ObjectBytes(detail::console_data::TEXT_INK),                   0x0000,     offsetof(OpenDataSegment, text_ink) },
  { ObjectBytes(detail::console_data::TEXT_BACKGROUND),            0x0000,     offsetof(OpenDataSegment, text_background) },
  { ObjectBytes(detail::console_data::TEXT_LARGE),                 0x0000,     offsetof(OpenDataSegment, text_large) },
  { ObjectBytes(detail::console_data::TEXT_SAVED),                 0x0000,     offsetof(OpenDataSegment, text_saved) },
  { ObjectBytes(detail::console_data::TEXT_LEAD),                  0x0000,     offsetof(OpenDataSegment, text_lead) },
  { ObjectBytes(detail::console_data::TEXT_STATE),                 0x0000,     offsetof(OpenDataSegment, text_state) },
  { ObjectBytes(detail::console_data::FORMAT_OUTPUT),              0x0000,     offsetof(OpenDataSegment, format_output) },
  // Reserved: kept for the layout until M4
  { ObjectBytes(detail::scene_data::RESERVED_EMS_NAME),            0x0000,     offsetof(OpenDataSegment, reserved_ems_name) },
  { ObjectBytes(detail::scene_data::RESERVED_SPRITE_MAPPED),       0x0000,     offsetof(OpenDataSegment, reserved_sprite_mapped) },
  // The music
  { ObjectBytes(detail::music_data::MUSIC_CURRENT),                0x0000,     offsetof(OpenDataSegment, music_current) },
  { ObjectBytes(detail::music_data::EFFECT_CURRENT),               0x0000,     offsetof(OpenDataSegment, effect_current) },
  { ObjectBytes(detail::music_data::MUSIC_BANK),                   0x0000,     offsetof(OpenDataSegment, music_bank) },
  { ObjectBytes(detail::music_data::MUSIC_PATH),                   0x0000,     offsetof(OpenDataSegment, music_path) },
  // The scenes
  { ObjectBytes(detail::scene_data::ABORTED),                      0x0000,     offsetof(OpenDataSegment, aborted) },
  { ObjectBytes(detail::scene_data::FRAME_Y),                      0x0000,     offsetof(OpenDataSegment, frame_y) },
  { ObjectBytes(detail::scene_data::FRAME_H),                      0x0000,     offsetof(OpenDataSegment, frame_h) },
  { ObjectBytes(detail::scene_data::FRAME_OFF),                    0x0000,     offsetof(OpenDataSegment, frame_off) },
  { ObjectBytes(detail::scene_data::BOB_A),                        0x0000,     offsetof(OpenDataSegment, bob_a) },
  { ObjectBytes(detail::scene_data::BLAST_STEPS),                  0x0000,     offsetof(OpenDataSegment, blast_steps) },
  { ObjectBytes(detail::scene_data::BLAST_SLOTS),                  0x0000,     offsetof(OpenDataSegment, blast_slots) },
  { ObjectBytes(detail::scene_data::BLAST_X),                      0x0000,     offsetof(OpenDataSegment, blast_x) },
  { ObjectBytes(detail::scene_data::BLAST_Y),                      0x0000,     offsetof(OpenDataSegment, blast_y) },
  { ObjectBytes(detail::scene_data::BLAST_MASKS),                  0x0000,     offsetof(OpenDataSegment, blast_masks) },
  { ObjectBytes(detail::scene_data::BLAST_SPRITES),                0x0000,     offsetof(OpenDataSegment, blast_sprites) },
  { ObjectBytes(detail::scene_data::FLASH_SLOTS),                  0x0000,     offsetof(OpenDataSegment, flash_slots) },
  { ObjectBytes(detail::scene_data::BOB_B),                        0x0000,     offsetof(OpenDataSegment, bob_b) },
  { ObjectBytes(detail::scene_data::FLAK_X),                       0x0000,     offsetof(OpenDataSegment, flak_x) },
  { ObjectBytes(detail::scene_data::FLAK_Y),                       0x0000,     offsetof(OpenDataSegment, flak_y) },
  { ObjectBytes(detail::scene_data::GUN_FLIPS),                    0x0000,     offsetof(OpenDataSegment, gun_flips) },
  { ObjectBytes(detail::scene_data::SHELL_X),                      0x0000,     offsetof(OpenDataSegment, shell_x) },
  { ObjectBytes(detail::scene_data::SHELL_Y),                      0x0000,     offsetof(OpenDataSegment, shell_y) },
  { ObjectBytes(detail::scene_data::SHELL_W),                      0x0000,     offsetof(OpenDataSegment, shell_w) },
  { ObjectBytes(detail::scene_data::SHELL_WS),                     0x0000,     offsetof(OpenDataSegment, shell_ws) },
  { ObjectBytes(detail::scene_data::SHELL_H),                      0x0000,     offsetof(OpenDataSegment, shell_h) },
  { ObjectBytes(detail::scene_data::SHELL_SPRITE),                 0x0000,     offsetof(OpenDataSegment, shell_sprite) },
  { ObjectBytes(detail::scene_data::SHELL_SAVE),                   0x0000,     offsetof(OpenDataSegment, shell_save) },
  { ObjectBytes(detail::scene_data::FLASH_ON),                     0x0000,     offsetof(OpenDataSegment, flash_on) },
  { ObjectBytes(detail::scene_data::FLASH_PHASE),                  0x0000,     offsetof(OpenDataSegment, flash_phase) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR1),                   0x0000,     offsetof(OpenDataSegment, alloc_err1) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR2),                   0x0000,     offsetof(OpenDataSegment, alloc_err2) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR3),                   0x0000,     offsetof(OpenDataSegment, alloc_err3) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR4),                   0x0000,     offsetof(OpenDataSegment, alloc_err4) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR5),                   0x0000,     offsetof(OpenDataSegment, alloc_err5) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR6),                   0x0000,     offsetof(OpenDataSegment, alloc_err6) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR7),                   0x0000,     offsetof(OpenDataSegment, alloc_err7) },
  { ObjectBytes(detail::scene_data::ALLOC_ERR9),                   0x0000,     offsetof(OpenDataSegment, alloc_err9) },
  { ObjectBytes(detail::scene_data::CONSOLE_RESET),                0x0000,     offsetof(OpenDataSegment, console_reset) },
  { ObjectBytes(detail::scene_data::SCENARIO_FILE_NAME),           0x0000,     offsetof(OpenDataSegment, scenario_file_name) },
  // The helpers
  { ObjectBytes(detail::helper_data::STREAM_COUNT),                0x0000,     offsetof(OpenDataSegment, stream_count) },
  { ObjectBytes(detail::helper_data::PALETTE),                     0x0000,     offsetof(OpenDataSegment, palette) },
  { ObjectBytes(detail::helper_data::BLACK_PALETTE),               0x0000,     offsetof(OpenDataSegment, black) },
  { ObjectBytes(detail::helper_data::SPARE_PALETTE),               0x0000,     offsetof(OpenDataSegment, spare_palette) },
  { ObjectBytes(detail::helper_data::ICON),                        0x0000,     offsetof(OpenDataSegment, icon) },
  { ObjectBytes(detail::helper_data::ERROR_FORMAT),                0x0000,     offsetof(OpenDataSegment, error_format) },
  // The file names and the palette table
  { ObjectBytes(detail::file_and_palette_data::LOGO_FILE_NAME),    0x0000,     offsetof(OpenDataSegment, logo_file_name) },
  { ObjectBytes(detail::file_and_palette_data::PICTURE_FILE_NAME), 0x0000,     offsetof(OpenDataSegment, picture_file_name) },
  { ObjectBytes(detail::file_and_palette_data::SPRITE_FILE_NAME),  0x0000,     offsetof(OpenDataSegment, sprite_file_name) },
  { ObjectBytes(detail::file_and_palette_data::PICTURE_HANDLE),    0x0000,     offsetof(OpenDataSegment, picture_handle) },
  { ObjectBytes(detail::file_and_palette_data::LOGO_HANDLE),       0x0000,     offsetof(OpenDataSegment, logo_handle) },
  { ObjectBytes(detail::file_and_palette_data::PALETTE_TABLE),     0x0000,     offsetof(OpenDataSegment, palettes) },
  { ObjectBytes(detail::file_and_palette_data::CLOSING_PALETTE),   0x0000,     offsetof(OpenDataSegment, closing_palette) },
  // OFMUSIC
  { ObjectBytes(detail::ofmusic::opeu_music_tracks),               BANK_FRAME, BANK_OFFSET },
  { ObjectBytes(detail::ofmusic::opeu_music_voices),               BANK_FRAME, 0x0020 },
  { ObjectBytes(detail::ofmusic::opeu_music_events),               BANK_FRAME, 0x01df },
} };
// clang-format on

// Each DGROUP object fills its field.
static_assert(sizeof detail::console_data::UNUSED_DIGIT_FONT == sizeof(OpenDataSegment::digit_font));
static_assert(sizeof detail::console_data::DRAW_HOOKS == sizeof(OpenDataSegment::draw_hooks));
static_assert(sizeof detail::console_data::TEXT_ERASE == sizeof(OpenDataSegment::text_erase));
static_assert(sizeof detail::console_data::TEXT_X == sizeof(OpenDataSegment::text_x));
static_assert(sizeof detail::console_data::TEXT_Y == sizeof(OpenDataSegment::text_y));
static_assert(sizeof detail::console_data::TEXT_LEFT == sizeof(OpenDataSegment::text_left));
static_assert(sizeof detail::console_data::TEXT_PAPER == sizeof(OpenDataSegment::text_paper));
static_assert(sizeof detail::console_data::TEXT_INK == sizeof(OpenDataSegment::text_ink));
static_assert(sizeof detail::console_data::TEXT_BACKGROUND == sizeof(OpenDataSegment::text_background));
static_assert(sizeof detail::console_data::TEXT_LARGE == sizeof(OpenDataSegment::text_large));
static_assert(sizeof detail::console_data::TEXT_SAVED == sizeof(OpenDataSegment::text_saved));
static_assert(sizeof detail::console_data::TEXT_LEAD == sizeof(OpenDataSegment::text_lead));
static_assert(sizeof detail::console_data::TEXT_STATE == sizeof(OpenDataSegment::text_state));
static_assert(sizeof detail::console_data::FORMAT_OUTPUT == sizeof(OpenDataSegment::format_output));
static_assert(sizeof detail::scene_data::RESERVED_EMS_NAME == sizeof(OpenDataSegment::reserved_ems_name));
static_assert(sizeof detail::scene_data::RESERVED_SPRITE_MAPPED == sizeof(OpenDataSegment::reserved_sprite_mapped));
static_assert(sizeof detail::music_data::MUSIC_CURRENT == sizeof(OpenDataSegment::music_current));
static_assert(sizeof detail::music_data::EFFECT_CURRENT == sizeof(OpenDataSegment::effect_current));
static_assert(sizeof detail::music_data::MUSIC_BANK == sizeof(OpenDataSegment::music_bank));
static_assert(sizeof detail::music_data::MUSIC_PATH == sizeof(OpenDataSegment::music_path));
static_assert(sizeof detail::scene_data::ABORTED == sizeof(OpenDataSegment::aborted));
static_assert(sizeof detail::scene_data::FRAME_Y == sizeof(OpenDataSegment::frame_y));
static_assert(sizeof detail::scene_data::FRAME_H == sizeof(OpenDataSegment::frame_h));
static_assert(sizeof detail::scene_data::FRAME_OFF == sizeof(OpenDataSegment::frame_off));
static_assert(sizeof detail::scene_data::BOB_A == sizeof(OpenDataSegment::bob_a));
static_assert(sizeof detail::scene_data::BLAST_STEPS == sizeof(OpenDataSegment::blast_steps));
static_assert(sizeof detail::scene_data::BLAST_SLOTS == sizeof(OpenDataSegment::blast_slots));
static_assert(sizeof detail::scene_data::BLAST_X == sizeof(OpenDataSegment::blast_x));
static_assert(sizeof detail::scene_data::BLAST_Y == sizeof(OpenDataSegment::blast_y));
static_assert(sizeof detail::scene_data::BLAST_MASKS == sizeof(OpenDataSegment::blast_masks));
static_assert(sizeof detail::scene_data::BLAST_SPRITES == sizeof(OpenDataSegment::blast_sprites));
static_assert(sizeof detail::scene_data::FLASH_SLOTS == sizeof(OpenDataSegment::flash_slots));
static_assert(sizeof detail::scene_data::BOB_B == sizeof(OpenDataSegment::bob_b));
static_assert(sizeof detail::scene_data::FLAK_X == sizeof(OpenDataSegment::flak_x));
static_assert(sizeof detail::scene_data::FLAK_Y == sizeof(OpenDataSegment::flak_y));
static_assert(sizeof detail::scene_data::GUN_FLIPS == sizeof(OpenDataSegment::gun_flips));
static_assert(sizeof detail::scene_data::SHELL_X == sizeof(OpenDataSegment::shell_x));
static_assert(sizeof detail::scene_data::SHELL_Y == sizeof(OpenDataSegment::shell_y));
static_assert(sizeof detail::scene_data::SHELL_W == sizeof(OpenDataSegment::shell_w));
static_assert(sizeof detail::scene_data::SHELL_WS == sizeof(OpenDataSegment::shell_ws));
static_assert(sizeof detail::scene_data::SHELL_H == sizeof(OpenDataSegment::shell_h));
static_assert(sizeof detail::scene_data::SHELL_SPRITE == sizeof(OpenDataSegment::shell_sprite));
static_assert(sizeof detail::scene_data::SHELL_SAVE == sizeof(OpenDataSegment::shell_save));
static_assert(sizeof detail::scene_data::FLASH_ON == sizeof(OpenDataSegment::flash_on));
static_assert(sizeof detail::scene_data::FLASH_PHASE == sizeof(OpenDataSegment::flash_phase));
static_assert(sizeof detail::scene_data::ALLOC_ERR1 == sizeof(OpenDataSegment::alloc_err1));
static_assert(sizeof detail::scene_data::ALLOC_ERR2 == sizeof(OpenDataSegment::alloc_err2));
static_assert(sizeof detail::scene_data::ALLOC_ERR3 == sizeof(OpenDataSegment::alloc_err3));
static_assert(sizeof detail::scene_data::ALLOC_ERR4 == sizeof(OpenDataSegment::alloc_err4));
static_assert(sizeof detail::scene_data::ALLOC_ERR5 == sizeof(OpenDataSegment::alloc_err5));
static_assert(sizeof detail::scene_data::ALLOC_ERR6 == sizeof(OpenDataSegment::alloc_err6));
static_assert(sizeof detail::scene_data::ALLOC_ERR7 == sizeof(OpenDataSegment::alloc_err7));
static_assert(sizeof detail::scene_data::ALLOC_ERR9 == sizeof(OpenDataSegment::alloc_err9));
static_assert(sizeof detail::scene_data::CONSOLE_RESET == sizeof(OpenDataSegment::console_reset));
static_assert(sizeof detail::scene_data::SCENARIO_FILE_NAME == sizeof(OpenDataSegment::scenario_file_name));
static_assert(sizeof detail::helper_data::STREAM_COUNT == sizeof(OpenDataSegment::stream_count));
static_assert(sizeof detail::helper_data::PALETTE == sizeof(OpenDataSegment::palette));
static_assert(sizeof detail::helper_data::BLACK_PALETTE == sizeof(OpenDataSegment::black));
static_assert(sizeof detail::helper_data::SPARE_PALETTE == sizeof(OpenDataSegment::spare_palette));
static_assert(sizeof detail::helper_data::ICON == sizeof(OpenDataSegment::icon));
static_assert(sizeof detail::helper_data::ERROR_FORMAT == sizeof(OpenDataSegment::error_format));
static_assert(sizeof detail::file_and_palette_data::LOGO_FILE_NAME == sizeof(OpenDataSegment::logo_file_name));
static_assert(sizeof detail::file_and_palette_data::PICTURE_FILE_NAME == sizeof(OpenDataSegment::picture_file_name));
static_assert(sizeof detail::file_and_palette_data::SPRITE_FILE_NAME == sizeof(OpenDataSegment::sprite_file_name));
static_assert(sizeof detail::file_and_palette_data::PICTURE_HANDLE == sizeof(OpenDataSegment::picture_handle));
static_assert(sizeof detail::file_and_palette_data::LOGO_HANDLE == sizeof(OpenDataSegment::logo_handle));
static_assert(sizeof detail::file_and_palette_data::PALETTE_TABLE == sizeof(OpenDataSegment::palettes));
static_assert(sizeof detail::file_and_palette_data::CLOSING_PALETTE == sizeof(OpenDataSegment::closing_palette));
} // namespace

auto OpenImageObjects() -> std::span<OpenImageObject const> {
  return IMAGE_OBJECTS;
}
} // namespace opeu::intro::detail::load_image
