// Placement of the recovered OPEN data objects (recovered/data): seg 0 = DGROUP at the offset of the object's field in
// OpenDataSegment, otherwise a segment relative to OPEN's load segment. OpenMemory::Load copies each object there.
#include "process.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace opeu::intro {
namespace {
// The bytes of a recovered data object.
template <class _Object>
auto ObjectBytes(_Object const& object) -> std::span<std::byte const> {
  return std::as_bytes(std::span{ &object, 1 });
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init): only addresses and sizes of the data objects, no values
std::array<OpenImageObject, 78> const IMAGE_OBJECTS{ {
  /* ODTEXT */
  { ObjectBytes(detail::odtext::opeu_open_digit_font),        0x0000,     offsetof(OpenDataSegment, digit_font) },
  { ObjectBytes(detail::odtext::opeu_open_draw_hooks),        0x0000,     offsetof(OpenDataSegment, draw_hooks) },
  { ObjectBytes(detail::odtext::opeu_text_erase),             0x0000,     offsetof(OpenDataSegment, text_erase) },
  { ObjectBytes(detail::odtext::opeu_text_x),                 0x0000,     offsetof(OpenDataSegment, text_x) },
  { ObjectBytes(detail::odtext::opeu_text_y),                 0x0000,     offsetof(OpenDataSegment, text_y) },
  { ObjectBytes(detail::odtext::opeu_text_left),              0x0000,     offsetof(OpenDataSegment, text_left) },
  { ObjectBytes(detail::odtext::opeu_text_paper),             0x0000,     offsetof(OpenDataSegment, text_paper) },
  { ObjectBytes(detail::odtext::opeu_text_ink),               0x0000,     offsetof(OpenDataSegment, text_ink) },
  { ObjectBytes(detail::odtext::opeu_text_background),        0x0000,     offsetof(OpenDataSegment, text_background) },
  { ObjectBytes(detail::odtext::opeu_text_large),             0x0000,     offsetof(OpenDataSegment, text_large) },
  { ObjectBytes(detail::odtext::opeu_text_saved),             0x0000,     offsetof(OpenDataSegment, text_saved) },
  { ObjectBytes(detail::odtext::opeu_text_lead),              0x0000,     offsetof(OpenDataSegment, text_lead) },
  { ObjectBytes(detail::odtext::opeu_text_state),             0x0000,     offsetof(OpenDataSegment, text_state) },
  { ObjectBytes(detail::odtext::opeu_format_output),          0x0000,     offsetof(OpenDataSegment, format_output) },
  /* Reserved: kept for the layout until M4 */
  { ObjectBytes(detail::odopen::opeu_reserved_ems_name),      0x0000,     offsetof(OpenDataSegment, reserved_ems_name) },
  { ObjectBytes(detail::odopen::opeu_reserved_sprite_mapped), 0x0000,     offsetof(OpenDataSegment, reserved_sprite_mapped) },
  /* ODSOUND */
  { ObjectBytes(detail::odsound::opeu_music_current),         0x0000,     offsetof(OpenDataSegment, music_current) },
  { ObjectBytes(detail::odsound::opeu_effect_current),        0x0000,     offsetof(OpenDataSegment, effect_current) },
  { ObjectBytes(detail::odsound::opeu_music_bank),            0x0000,     offsetof(OpenDataSegment, music_bank) },
  { ObjectBytes(detail::odsound::opeu_music_path),            0x0000,     offsetof(OpenDataSegment, music_path) },
  /* ODOPEN */
  { ObjectBytes(detail::odopen::opeu_open_aborted),           0x0000,     offsetof(OpenDataSegment, aborted) },
  { ObjectBytes(detail::odopen::opeu_open_frame_y),           0x0000,     offsetof(OpenDataSegment, frame_y) },
  { ObjectBytes(detail::odopen::opeu_open_frame_h),           0x0000,     offsetof(OpenDataSegment, frame_h) },
  { ObjectBytes(detail::odopen::opeu_open_frame_off),         0x0000,     offsetof(OpenDataSegment, frame_off) },
  { ObjectBytes(detail::odopen::opeu_open_bob_a),             0x0000,     offsetof(OpenDataSegment, bob_a) },
  { ObjectBytes(detail::odopen::opeu_open_blast_steps),       0x0000,     offsetof(OpenDataSegment, blast_steps) },
  { ObjectBytes(detail::odopen::opeu_open_blast_slots),       0x0000,     offsetof(OpenDataSegment, blast_slots) },
  { ObjectBytes(detail::odopen::opeu_open_blast_x),           0x0000,     offsetof(OpenDataSegment, blast_x) },
  { ObjectBytes(detail::odopen::opeu_open_blast_y),           0x0000,     offsetof(OpenDataSegment, blast_y) },
  { ObjectBytes(detail::odopen::opeu_open_blast_masks),       0x0000,     offsetof(OpenDataSegment, blast_masks) },
  { ObjectBytes(detail::odopen::opeu_open_blast_sprites),     0x0000,     offsetof(OpenDataSegment, blast_sprites) },
  { ObjectBytes(detail::odopen::opeu_open_flash_slots),       0x0000,     offsetof(OpenDataSegment, flash_slots) },
  { ObjectBytes(detail::odopen::opeu_open_bob_b),             0x0000,     offsetof(OpenDataSegment, bob_b) },
  { ObjectBytes(detail::odopen::opeu_open_flak_x),            0x0000,     offsetof(OpenDataSegment, flak_x) },
  { ObjectBytes(detail::odopen::opeu_open_flak_y),            0x0000,     offsetof(OpenDataSegment, flak_y) },
  { ObjectBytes(detail::odopen::opeu_open_gun_flips),         0x0000,     offsetof(OpenDataSegment, gun_flips) },
  { ObjectBytes(detail::odopen::opeu_open_shell_x),           0x0000,     offsetof(OpenDataSegment, shell_x) },
  { ObjectBytes(detail::odopen::opeu_open_shell_y),           0x0000,     offsetof(OpenDataSegment, shell_y) },
  { ObjectBytes(detail::odopen::opeu_open_shell_w),           0x0000,     offsetof(OpenDataSegment, shell_w) },
  { ObjectBytes(detail::odopen::opeu_open_shell_ws),          0x0000,     offsetof(OpenDataSegment, shell_ws) },
  { ObjectBytes(detail::odopen::opeu_open_shell_h),           0x0000,     offsetof(OpenDataSegment, shell_h) },
  { ObjectBytes(detail::odopen::opeu_open_shell_sprite),      0x0000,     offsetof(OpenDataSegment, shell_sprite) },
  { ObjectBytes(detail::odopen::opeu_open_shell_save),        0x0000,     offsetof(OpenDataSegment, shell_save) },
  { ObjectBytes(detail::odopen::opeu_open_flash_on),          0x0000,     offsetof(OpenDataSegment, flash_on) },
  { ObjectBytes(detail::odopen::opeu_open_flash_phase),       0x0000,     offsetof(OpenDataSegment, flash_phase) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err1),   0x0000,     offsetof(OpenDataSegment, alloc_err1) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err2),   0x0000,     offsetof(OpenDataSegment, alloc_err2) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err3),   0x0000,     offsetof(OpenDataSegment, alloc_err3) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err4),   0x0000,     offsetof(OpenDataSegment, alloc_err4) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err5),   0x0000,     offsetof(OpenDataSegment, alloc_err5) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err6),   0x0000,     offsetof(OpenDataSegment, alloc_err6) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err7),   0x0000,     offsetof(OpenDataSegment, alloc_err7) },
  { ObjectBytes(detail::odopen::opeu_open_text_alloc_err9),   0x0000,     offsetof(OpenDataSegment, alloc_err9) },
  { ObjectBytes(detail::odopen::opeu_open_text_console),      0x0000,     offsetof(OpenDataSegment, text_console) },
  { ObjectBytes(detail::odopen::opeu_open_name_scesel),       0x0000,     offsetof(OpenDataSegment, name_scesel) },
  /* ODUTIL */
  { ObjectBytes(detail::odutil::opeu_stream_count),           0x0000,     offsetof(OpenDataSegment, stream_count) },
  { ObjectBytes(detail::odutil::opeu_open_palette),           0x0000,     offsetof(OpenDataSegment, palette) },
  { ObjectBytes(detail::odutil::opeu_open_black),             0x0000,     offsetof(OpenDataSegment, black) },
  { ObjectBytes(detail::odutil::opeu_open_spare_palette),     0x0000,     offsetof(OpenDataSegment, spare_palette) },
  { ObjectBytes(detail::odutil::opeu_open_icon),              0x0000,     offsetof(OpenDataSegment, icon) },
  { ObjectBytes(detail::odutil::opeu_open_error_format),      0x0000,     offsetof(OpenDataSegment, error_format) },
  /* ODPAL */
  { ObjectBytes(detail::odpal::opeu_open_name_fnt),           0x0000,     offsetof(OpenDataSegment, name_fnt) },
  { ObjectBytes(detail::odpal::opeu_open_name_npk),           0x0000,     offsetof(OpenDataSegment, name_npk) },
  { ObjectBytes(detail::odpal::opeu_open_name_mopengrp),      0x0000,     offsetof(OpenDataSegment, name_mopengrp) },
  { ObjectBytes(detail::odpal::opeu_picture_handle),          0x0000,     offsetof(OpenDataSegment, picture_handle) },
  { ObjectBytes(detail::odpal::opeu_open_logo_file),          0x0000,     offsetof(OpenDataSegment, logo_file) },
  { ObjectBytes(detail::odpal::opeu_open_palettes),           0x0000,     offsetof(OpenDataSegment, palettes) },
  { ObjectBytes(detail::odpal::opeu_open_fade_target),        0x0000,     offsetof(OpenDataSegment, fade_target) },
  /* OFMUSIC */
  { ObjectBytes(detail::ofmusic::opeu_music_tracks),          BANK_FRAME, BANK_OFFSET },
  { ObjectBytes(detail::ofmusic::opeu_music_voices),          BANK_FRAME, 0x0020 },
  { ObjectBytes(detail::ofmusic::opeu_music_events),          BANK_FRAME, 0x01df },
} };
// clang-format on

/* Each DGROUP object fills its field. */
static_assert(sizeof detail::odtext::opeu_open_digit_font == sizeof(OpenDataSegment::digit_font));
static_assert(sizeof detail::odtext::opeu_open_draw_hooks == sizeof(OpenDataSegment::draw_hooks));
static_assert(sizeof detail::odtext::opeu_text_erase == sizeof(OpenDataSegment::text_erase));
static_assert(sizeof detail::odtext::opeu_text_x == sizeof(OpenDataSegment::text_x));
static_assert(sizeof detail::odtext::opeu_text_y == sizeof(OpenDataSegment::text_y));
static_assert(sizeof detail::odtext::opeu_text_left == sizeof(OpenDataSegment::text_left));
static_assert(sizeof detail::odtext::opeu_text_paper == sizeof(OpenDataSegment::text_paper));
static_assert(sizeof detail::odtext::opeu_text_ink == sizeof(OpenDataSegment::text_ink));
static_assert(sizeof detail::odtext::opeu_text_background == sizeof(OpenDataSegment::text_background));
static_assert(sizeof detail::odtext::opeu_text_large == sizeof(OpenDataSegment::text_large));
static_assert(sizeof detail::odtext::opeu_text_saved == sizeof(OpenDataSegment::text_saved));
static_assert(sizeof detail::odtext::opeu_text_lead == sizeof(OpenDataSegment::text_lead));
static_assert(sizeof detail::odtext::opeu_text_state == sizeof(OpenDataSegment::text_state));
static_assert(sizeof detail::odtext::opeu_format_output == sizeof(OpenDataSegment::format_output));
static_assert(sizeof detail::odopen::opeu_reserved_ems_name == sizeof(OpenDataSegment::reserved_ems_name));
static_assert(sizeof detail::odopen::opeu_reserved_sprite_mapped == sizeof(OpenDataSegment::reserved_sprite_mapped));
static_assert(sizeof detail::odsound::opeu_music_current == sizeof(OpenDataSegment::music_current));
static_assert(sizeof detail::odsound::opeu_effect_current == sizeof(OpenDataSegment::effect_current));
static_assert(sizeof detail::odsound::opeu_music_bank == sizeof(OpenDataSegment::music_bank));
static_assert(sizeof detail::odsound::opeu_music_path == sizeof(OpenDataSegment::music_path));
static_assert(sizeof detail::odopen::opeu_open_aborted == sizeof(OpenDataSegment::aborted));
static_assert(sizeof detail::odopen::opeu_open_frame_y == sizeof(OpenDataSegment::frame_y));
static_assert(sizeof detail::odopen::opeu_open_frame_h == sizeof(OpenDataSegment::frame_h));
static_assert(sizeof detail::odopen::opeu_open_frame_off == sizeof(OpenDataSegment::frame_off));
static_assert(sizeof detail::odopen::opeu_open_bob_a == sizeof(OpenDataSegment::bob_a));
static_assert(sizeof detail::odopen::opeu_open_blast_steps == sizeof(OpenDataSegment::blast_steps));
static_assert(sizeof detail::odopen::opeu_open_blast_slots == sizeof(OpenDataSegment::blast_slots));
static_assert(sizeof detail::odopen::opeu_open_blast_x == sizeof(OpenDataSegment::blast_x));
static_assert(sizeof detail::odopen::opeu_open_blast_y == sizeof(OpenDataSegment::blast_y));
static_assert(sizeof detail::odopen::opeu_open_blast_masks == sizeof(OpenDataSegment::blast_masks));
static_assert(sizeof detail::odopen::opeu_open_blast_sprites == sizeof(OpenDataSegment::blast_sprites));
static_assert(sizeof detail::odopen::opeu_open_flash_slots == sizeof(OpenDataSegment::flash_slots));
static_assert(sizeof detail::odopen::opeu_open_bob_b == sizeof(OpenDataSegment::bob_b));
static_assert(sizeof detail::odopen::opeu_open_flak_x == sizeof(OpenDataSegment::flak_x));
static_assert(sizeof detail::odopen::opeu_open_flak_y == sizeof(OpenDataSegment::flak_y));
static_assert(sizeof detail::odopen::opeu_open_gun_flips == sizeof(OpenDataSegment::gun_flips));
static_assert(sizeof detail::odopen::opeu_open_shell_x == sizeof(OpenDataSegment::shell_x));
static_assert(sizeof detail::odopen::opeu_open_shell_y == sizeof(OpenDataSegment::shell_y));
static_assert(sizeof detail::odopen::opeu_open_shell_w == sizeof(OpenDataSegment::shell_w));
static_assert(sizeof detail::odopen::opeu_open_shell_ws == sizeof(OpenDataSegment::shell_ws));
static_assert(sizeof detail::odopen::opeu_open_shell_h == sizeof(OpenDataSegment::shell_h));
static_assert(sizeof detail::odopen::opeu_open_shell_sprite == sizeof(OpenDataSegment::shell_sprite));
static_assert(sizeof detail::odopen::opeu_open_shell_save == sizeof(OpenDataSegment::shell_save));
static_assert(sizeof detail::odopen::opeu_open_flash_on == sizeof(OpenDataSegment::flash_on));
static_assert(sizeof detail::odopen::opeu_open_flash_phase == sizeof(OpenDataSegment::flash_phase));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err1 == sizeof(OpenDataSegment::alloc_err1));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err2 == sizeof(OpenDataSegment::alloc_err2));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err3 == sizeof(OpenDataSegment::alloc_err3));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err4 == sizeof(OpenDataSegment::alloc_err4));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err5 == sizeof(OpenDataSegment::alloc_err5));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err6 == sizeof(OpenDataSegment::alloc_err6));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err7 == sizeof(OpenDataSegment::alloc_err7));
static_assert(sizeof detail::odopen::opeu_open_text_alloc_err9 == sizeof(OpenDataSegment::alloc_err9));
static_assert(sizeof detail::odopen::opeu_open_text_console == sizeof(OpenDataSegment::text_console));
static_assert(sizeof detail::odopen::opeu_open_name_scesel == sizeof(OpenDataSegment::name_scesel));
static_assert(sizeof detail::odutil::opeu_stream_count == sizeof(OpenDataSegment::stream_count));
static_assert(sizeof detail::odutil::opeu_open_palette == sizeof(OpenDataSegment::palette));
static_assert(sizeof detail::odutil::opeu_open_black == sizeof(OpenDataSegment::black));
static_assert(sizeof detail::odutil::opeu_open_spare_palette == sizeof(OpenDataSegment::spare_palette));
static_assert(sizeof detail::odutil::opeu_open_icon == sizeof(OpenDataSegment::icon));
static_assert(sizeof detail::odutil::opeu_open_error_format == sizeof(OpenDataSegment::error_format));
static_assert(sizeof detail::odpal::opeu_open_name_fnt == sizeof(OpenDataSegment::name_fnt));
static_assert(sizeof detail::odpal::opeu_open_name_npk == sizeof(OpenDataSegment::name_npk));
static_assert(sizeof detail::odpal::opeu_open_name_mopengrp == sizeof(OpenDataSegment::name_mopengrp));
static_assert(sizeof detail::odpal::opeu_picture_handle == sizeof(OpenDataSegment::picture_handle));
static_assert(sizeof detail::odpal::opeu_open_logo_file == sizeof(OpenDataSegment::logo_file));
static_assert(sizeof detail::odpal::opeu_open_palettes == sizeof(OpenDataSegment::palettes));
static_assert(sizeof detail::odpal::opeu_open_fade_target == sizeof(OpenDataSegment::fade_target));
} // namespace

auto OpenImageObjects() -> std::span<OpenImageObject const> {
  return IMAGE_OBJECTS;
}
} // namespace opeu::intro
