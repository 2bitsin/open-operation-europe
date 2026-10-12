/* Placement of the recovered OPEN data objects (recovered/data): seg 0 =
 * DGROUP at the offset of the object's field in OpenDataSegment, otherwise a
 * segment relative to OPEN's load segment. memory.cpp copies each object
 * there. */
#include "process.hpp"

#include <cstddef>

namespace opeu::intro {

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init): only addresses and sizes of the data objects, no values
OpenImageObject const open_image_objects[] = {
  /* ODTEXT */
  { &detail::odtext::opeu_open_digit_font, 0x0000, offsetof(OpenDataSegment, digit_font),
   sizeof detail::odtext::opeu_open_digit_font, "opeu_open_digit_font" },
  { &detail::odtext::opeu_open_draw_hooks, 0x0000, offsetof(OpenDataSegment, draw_hooks),
   sizeof detail::odtext::opeu_open_draw_hooks, "opeu_open_draw_hooks" },
  { &detail::odtext::opeu_text_erase, 0x0000, offsetof(OpenDataSegment, text_erase),
   sizeof detail::odtext::opeu_text_erase, "opeu_text_erase" },
  { &detail::odtext::opeu_text_unused_014a, 0x0000, offsetof(OpenDataSegment, text_unused_014a),
   sizeof detail::odtext::opeu_text_unused_014a, "opeu_text_unused_014a" },
  { &detail::odtext::opeu_text_x, 0x0000, offsetof(OpenDataSegment, text_x), sizeof detail::odtext::opeu_text_x,
   "opeu_text_x" },
  { &detail::odtext::opeu_text_y, 0x0000, offsetof(OpenDataSegment, text_y), sizeof detail::odtext::opeu_text_y,
   "opeu_text_y" },
  { &detail::odtext::opeu_text_left, 0x0000, offsetof(OpenDataSegment, text_left),
   sizeof detail::odtext::opeu_text_left, "opeu_text_left" },
  { &detail::odtext::opeu_text_paper, 0x0000, offsetof(OpenDataSegment, text_paper),
   sizeof detail::odtext::opeu_text_paper, "opeu_text_paper" },
  { &detail::odtext::opeu_text_ink, 0x0000, offsetof(OpenDataSegment, text_ink), sizeof detail::odtext::opeu_text_ink,
   "opeu_text_ink" },
  { &detail::odtext::opeu_text_background, 0x0000, offsetof(OpenDataSegment, text_background),
   sizeof detail::odtext::opeu_text_background, "opeu_text_background" },
  { &detail::odtext::opeu_text_z_0163, 0x0000, offsetof(OpenDataSegment, text_z_0163),
   sizeof detail::odtext::opeu_text_z_0163, "opeu_text_z_0163" },
  { &detail::odtext::opeu_text_large, 0x0000, offsetof(OpenDataSegment, text_large),
   sizeof detail::odtext::opeu_text_large, "opeu_text_large" },
  { &detail::odtext::opeu_text_saved, 0x0000, offsetof(OpenDataSegment, text_saved),
   sizeof detail::odtext::opeu_text_saved, "opeu_text_saved" },
  { &detail::odtext::opeu_text_lead, 0x0000, offsetof(OpenDataSegment, text_lead),
   sizeof detail::odtext::opeu_text_lead, "opeu_text_lead" },
  { &detail::odtext::opeu_text_z_0173, 0x0000, offsetof(OpenDataSegment, text_z_0173),
   sizeof detail::odtext::opeu_text_z_0173, "opeu_text_z_0173" },
  { &detail::odtext::opeu_text_state, 0x0000, offsetof(OpenDataSegment, text_state),
   sizeof detail::odtext::opeu_text_state, "opeu_text_state" },
  { &detail::odtext::opeu_format_output, 0x0000, offsetof(OpenDataSegment, format_output),
   sizeof detail::odtext::opeu_format_output, "opeu_format_output" },
  /* ODEMS */
  { &detail::odems::opeu_ems_device_name, 0x0000, offsetof(OpenDataSegment, ems_device_name),
   sizeof detail::odems::opeu_ems_device_name, "opeu_ems_device_name" },
  { &detail::odems::opeu_sprite_mapped, 0x0000, offsetof(OpenDataSegment, sprite_mapped),
   sizeof detail::odems::opeu_sprite_mapped, "opeu_sprite_mapped" },
  /* ODSOUND */
  { &detail::odsound::opeu_music_current, 0x0000, offsetof(OpenDataSegment, music_current),
   sizeof detail::odsound::opeu_music_current, "opeu_music_current" },
  { &detail::odsound::opeu_effect_current, 0x0000, offsetof(OpenDataSegment, effect_current),
   sizeof detail::odsound::opeu_effect_current, "opeu_effect_current" },
  { &detail::odsound::opeu_music_bank, 0x0000, offsetof(OpenDataSegment, music_bank),
   sizeof detail::odsound::opeu_music_bank, "opeu_music_bank" },
  { &detail::odsound::opeu_music_path, 0x0000, offsetof(OpenDataSegment, music_path),
   sizeof detail::odsound::opeu_music_path, "opeu_music_path" },
  /* ODOPEN */
  { &detail::odopen::opeu_open_aborted, 0x0000, offsetof(OpenDataSegment, aborted),
   sizeof detail::odopen::opeu_open_aborted, "opeu_open_aborted" },
  { &detail::odopen::opeu_open_frame_y, 0x0000, offsetof(OpenDataSegment, frame_y),
   sizeof detail::odopen::opeu_open_frame_y, "opeu_open_frame_y" },
  { &detail::odopen::opeu_open_frame_h, 0x0000, offsetof(OpenDataSegment, frame_h),
   sizeof detail::odopen::opeu_open_frame_h, "opeu_open_frame_h" },
  { &detail::odopen::opeu_open_frame_off, 0x0000, offsetof(OpenDataSegment, frame_off),
   sizeof detail::odopen::opeu_open_frame_off, "opeu_open_frame_off" },
  { &detail::odopen::opeu_open_bob_a, 0x0000, offsetof(OpenDataSegment, bob_a), sizeof detail::odopen::opeu_open_bob_a,
   "opeu_open_bob_a" },
  { &detail::odopen::opeu_open_blast_steps, 0x0000, offsetof(OpenDataSegment, blast_steps),
   sizeof detail::odopen::opeu_open_blast_steps, "opeu_open_blast_steps" },
  { &detail::odopen::opeu_open_blast_slots, 0x0000, offsetof(OpenDataSegment, blast_slots),
   sizeof detail::odopen::opeu_open_blast_slots, "opeu_open_blast_slots" },
  { &detail::odopen::opeu_open_z_0227, 0x0000, offsetof(OpenDataSegment, z_0227),
   sizeof detail::odopen::opeu_open_z_0227, "opeu_open_z_0227" },
  { &detail::odopen::opeu_open_blast_x, 0x0000, offsetof(OpenDataSegment, blast_x),
   sizeof detail::odopen::opeu_open_blast_x, "opeu_open_blast_x" },
  { &detail::odopen::opeu_open_blast_y, 0x0000, offsetof(OpenDataSegment, blast_y),
   sizeof detail::odopen::opeu_open_blast_y, "opeu_open_blast_y" },
  { &detail::odopen::opeu_open_blast_masks, 0x0000, offsetof(OpenDataSegment, blast_masks),
   sizeof detail::odopen::opeu_open_blast_masks, "opeu_open_blast_masks" },
  { &detail::odopen::opeu_open_blast_sprites, 0x0000, offsetof(OpenDataSegment, blast_sprites),
   sizeof detail::odopen::opeu_open_blast_sprites, "opeu_open_blast_sprites" },
  { &detail::odopen::opeu_open_flash_slots, 0x0000, offsetof(OpenDataSegment, flash_slots),
   sizeof detail::odopen::opeu_open_flash_slots, "opeu_open_flash_slots" },
  { &detail::odopen::opeu_open_z_024f, 0x0000, offsetof(OpenDataSegment, z_024f),
   sizeof detail::odopen::opeu_open_z_024f, "opeu_open_z_024f" },
  { &detail::odopen::opeu_open_bob_b, 0x0000, offsetof(OpenDataSegment, bob_b), sizeof detail::odopen::opeu_open_bob_b,
   "opeu_open_bob_b" },
  { &detail::odopen::opeu_open_flak_x, 0x0000, offsetof(OpenDataSegment, flak_x),
   sizeof detail::odopen::opeu_open_flak_x, "opeu_open_flak_x" },
  { &detail::odopen::opeu_open_flak_y, 0x0000, offsetof(OpenDataSegment, flak_y),
   sizeof detail::odopen::opeu_open_flak_y, "opeu_open_flak_y" },
  { &detail::odopen::opeu_open_gun_flips, 0x0000, offsetof(OpenDataSegment, gun_flips),
   sizeof detail::odopen::opeu_open_gun_flips, "opeu_open_gun_flips" },
  { &detail::odopen::opeu_open_shell_x, 0x0000, offsetof(OpenDataSegment, shell_x),
   sizeof detail::odopen::opeu_open_shell_x, "opeu_open_shell_x" },
  { &detail::odopen::opeu_open_shell_y, 0x0000, offsetof(OpenDataSegment, shell_y),
   sizeof detail::odopen::opeu_open_shell_y, "opeu_open_shell_y" },
  { &detail::odopen::opeu_open_shell_w, 0x0000, offsetof(OpenDataSegment, shell_w),
   sizeof detail::odopen::opeu_open_shell_w, "opeu_open_shell_w" },
  { &detail::odopen::opeu_open_shell_ws, 0x0000, offsetof(OpenDataSegment, shell_ws),
   sizeof detail::odopen::opeu_open_shell_ws, "opeu_open_shell_ws" },
  { &detail::odopen::opeu_open_shell_h, 0x0000, offsetof(OpenDataSegment, shell_h),
   sizeof detail::odopen::opeu_open_shell_h, "opeu_open_shell_h" },
  { &detail::odopen::opeu_open_shell_sprite, 0x0000, offsetof(OpenDataSegment, shell_sprite),
   sizeof detail::odopen::opeu_open_shell_sprite, "opeu_open_shell_sprite" },
  { &detail::odopen::opeu_open_shell_save, 0x0000, offsetof(OpenDataSegment, shell_save),
   sizeof detail::odopen::opeu_open_shell_save, "opeu_open_shell_save" },
  { &detail::odopen::opeu_open_flash_on, 0x0000, offsetof(OpenDataSegment, flash_on),
   sizeof detail::odopen::opeu_open_flash_on, "opeu_open_flash_on" },
  { &detail::odopen::opeu_open_flash_phase, 0x0000, offsetof(OpenDataSegment, flash_phase),
   sizeof detail::odopen::opeu_open_flash_phase, "opeu_open_flash_phase" },
  { &detail::odopen::opeu_open_text_alloc_err1, 0x0000, offsetof(OpenDataSegment, alloc_err1),
   sizeof detail::odopen::opeu_open_text_alloc_err1, "opeu_open_text_alloc_err1" },
  { &detail::odopen::opeu_open_text_alloc_err2, 0x0000, offsetof(OpenDataSegment, alloc_err2),
   sizeof detail::odopen::opeu_open_text_alloc_err2, "opeu_open_text_alloc_err2" },
  { &detail::odopen::opeu_open_text_alloc_err3, 0x0000, offsetof(OpenDataSegment, alloc_err3),
   sizeof detail::odopen::opeu_open_text_alloc_err3, "opeu_open_text_alloc_err3" },
  { &detail::odopen::opeu_open_text_alloc_err4, 0x0000, offsetof(OpenDataSegment, alloc_err4),
   sizeof detail::odopen::opeu_open_text_alloc_err4, "opeu_open_text_alloc_err4" },
  { &detail::odopen::opeu_open_text_alloc_err5, 0x0000, offsetof(OpenDataSegment, alloc_err5),
   sizeof detail::odopen::opeu_open_text_alloc_err5, "opeu_open_text_alloc_err5" },
  { &detail::odopen::opeu_open_text_alloc_err6, 0x0000, offsetof(OpenDataSegment, alloc_err6),
   sizeof detail::odopen::opeu_open_text_alloc_err6, "opeu_open_text_alloc_err6" },
  { &detail::odopen::opeu_open_text_alloc_err7, 0x0000, offsetof(OpenDataSegment, alloc_err7),
   sizeof detail::odopen::opeu_open_text_alloc_err7, "opeu_open_text_alloc_err7" },
  { &detail::odopen::opeu_open_text_alloc_err9, 0x0000, offsetof(OpenDataSegment, alloc_err9),
   sizeof detail::odopen::opeu_open_text_alloc_err9, "opeu_open_text_alloc_err9" },
  { &detail::odopen::opeu_open_text_console, 0x0000, offsetof(OpenDataSegment, text_console),
   sizeof detail::odopen::opeu_open_text_console, "opeu_open_text_console" },
  { &detail::odopen::opeu_open_name_scesel, 0x0000, offsetof(OpenDataSegment, name_scesel),
   sizeof detail::odopen::opeu_open_name_scesel, "opeu_open_name_scesel" },
  /* ODUTIL */
  { &detail::odutil::opeu_stream_count, 0x0000, offsetof(OpenDataSegment, stream_count),
   sizeof detail::odutil::opeu_stream_count, "opeu_stream_count" },
  { &detail::odutil::opeu_open_palette, 0x0000, offsetof(OpenDataSegment, palette),
   sizeof detail::odutil::opeu_open_palette, "opeu_open_palette" },
  { &detail::odutil::opeu_open_black, 0x0000, offsetof(OpenDataSegment, black), sizeof detail::odutil::opeu_open_black,
   "opeu_open_black" },
  { &detail::odutil::opeu_open_spare_palette, 0x0000, offsetof(OpenDataSegment, spare_palette),
   sizeof detail::odutil::opeu_open_spare_palette, "opeu_open_spare_palette" },
  { &detail::odutil::opeu_open_icon, 0x0000, offsetof(OpenDataSegment, icon), sizeof detail::odutil::opeu_open_icon,
   "opeu_open_icon" },
  { &detail::odutil::opeu_open_error_format, 0x0000, offsetof(OpenDataSegment, error_format),
   sizeof detail::odutil::opeu_open_error_format, "opeu_open_error_format" },
  /* ODPAL */
  { &detail::odpal::opeu_open_name_fnt, 0x0000, offsetof(OpenDataSegment, name_fnt),
   sizeof detail::odpal::opeu_open_name_fnt, "opeu_open_name_fnt" },
  { &detail::odpal::opeu_open_z_0425, 0x0000, offsetof(OpenDataSegment, z_0425), sizeof detail::odpal::opeu_open_z_0425,
   "opeu_open_z_0425" },
  { &detail::odpal::opeu_open_name_npk, 0x0000, offsetof(OpenDataSegment, name_npk),
   sizeof detail::odpal::opeu_open_name_npk, "opeu_open_name_npk" },
  { &detail::odpal::opeu_open_name_mopengrp, 0x0000, offsetof(OpenDataSegment, name_mopengrp),
   sizeof detail::odpal::opeu_open_name_mopengrp, "opeu_open_name_mopengrp" },
  { &detail::odpal::opeu_open_z_043f, 0x0000, offsetof(OpenDataSegment, z_043f), sizeof detail::odpal::opeu_open_z_043f,
   "opeu_open_z_043f" },
  { &detail::odpal::opeu_picture_handle, 0x0000, offsetof(OpenDataSegment, picture_handle),
   sizeof detail::odpal::opeu_picture_handle, "opeu_picture_handle" },
  { &detail::odpal::opeu_open_logo_file, 0x0000, offsetof(OpenDataSegment, logo_file),
   sizeof detail::odpal::opeu_open_logo_file, "opeu_open_logo_file" },
  { &detail::odpal::opeu_open_palettes, 0x0000, offsetof(OpenDataSegment, palettes),
   sizeof detail::odpal::opeu_open_palettes, "opeu_open_palettes" },
  { &detail::odpal::opeu_open_fade_target, 0x0000, offsetof(OpenDataSegment, fade_target),
   sizeof detail::odpal::opeu_open_fade_target, "opeu_open_fade_target" },
  /* OFMUSIC */
  { &detail::ofmusic::opeu_music_tracks, BANK_FRAME, BANK_OFFSET, sizeof detail::ofmusic::opeu_music_tracks,
   "opeu_music_tracks" },
  { &detail::ofmusic::opeu_music_voices, BANK_FRAME, 0x0020, sizeof detail::ofmusic::opeu_music_voices,
   "opeu_music_voices" },
  { &detail::ofmusic::opeu_music_events, BANK_FRAME, 0x01df, sizeof detail::ofmusic::opeu_music_events,
   "opeu_music_events" },
};

/* Each DGROUP object fills its field. */
static_assert(sizeof detail::odtext::opeu_open_digit_font == sizeof(OpenDataSegment::digit_font));
static_assert(sizeof detail::odtext::opeu_open_draw_hooks == sizeof(OpenDataSegment::draw_hooks));
static_assert(sizeof detail::odtext::opeu_text_erase == sizeof(OpenDataSegment::text_erase));
static_assert(sizeof detail::odtext::opeu_text_unused_014a == sizeof(OpenDataSegment::text_unused_014a));
static_assert(sizeof detail::odtext::opeu_text_x == sizeof(OpenDataSegment::text_x));
static_assert(sizeof detail::odtext::opeu_text_y == sizeof(OpenDataSegment::text_y));
static_assert(sizeof detail::odtext::opeu_text_left == sizeof(OpenDataSegment::text_left));
static_assert(sizeof detail::odtext::opeu_text_paper == sizeof(OpenDataSegment::text_paper));
static_assert(sizeof detail::odtext::opeu_text_ink == sizeof(OpenDataSegment::text_ink));
static_assert(sizeof detail::odtext::opeu_text_background == sizeof(OpenDataSegment::text_background));
static_assert(sizeof detail::odtext::opeu_text_z_0163 == sizeof(OpenDataSegment::text_z_0163));
static_assert(sizeof detail::odtext::opeu_text_large == sizeof(OpenDataSegment::text_large));
static_assert(sizeof detail::odtext::opeu_text_saved == sizeof(OpenDataSegment::text_saved));
static_assert(sizeof detail::odtext::opeu_text_lead == sizeof(OpenDataSegment::text_lead));
static_assert(sizeof detail::odtext::opeu_text_z_0173 == sizeof(OpenDataSegment::text_z_0173));
static_assert(sizeof detail::odtext::opeu_text_state == sizeof(OpenDataSegment::text_state));
static_assert(sizeof detail::odtext::opeu_format_output == sizeof(OpenDataSegment::format_output));
static_assert(sizeof detail::odems::opeu_ems_device_name == sizeof(OpenDataSegment::ems_device_name));
static_assert(sizeof detail::odems::opeu_sprite_mapped == sizeof(OpenDataSegment::sprite_mapped));
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
static_assert(sizeof detail::odopen::opeu_open_z_0227 == sizeof(OpenDataSegment::z_0227));
static_assert(sizeof detail::odopen::opeu_open_blast_x == sizeof(OpenDataSegment::blast_x));
static_assert(sizeof detail::odopen::opeu_open_blast_y == sizeof(OpenDataSegment::blast_y));
static_assert(sizeof detail::odopen::opeu_open_blast_masks == sizeof(OpenDataSegment::blast_masks));
static_assert(sizeof detail::odopen::opeu_open_blast_sprites == sizeof(OpenDataSegment::blast_sprites));
static_assert(sizeof detail::odopen::opeu_open_flash_slots == sizeof(OpenDataSegment::flash_slots));
static_assert(sizeof detail::odopen::opeu_open_z_024f == sizeof(OpenDataSegment::z_024f));
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
static_assert(sizeof detail::odpal::opeu_open_z_0425 == sizeof(OpenDataSegment::z_0425));
static_assert(sizeof detail::odpal::opeu_open_name_npk == sizeof(OpenDataSegment::name_npk));
static_assert(sizeof detail::odpal::opeu_open_name_mopengrp == sizeof(OpenDataSegment::name_mopengrp));
static_assert(sizeof detail::odpal::opeu_open_z_043f == sizeof(OpenDataSegment::z_043f));
static_assert(sizeof detail::odpal::opeu_picture_handle == sizeof(OpenDataSegment::picture_handle));
static_assert(sizeof detail::odpal::opeu_open_logo_file == sizeof(OpenDataSegment::logo_file));
static_assert(sizeof detail::odpal::opeu_open_palettes == sizeof(OpenDataSegment::palettes));
static_assert(sizeof detail::odpal::opeu_open_fade_target == sizeof(OpenDataSegment::fade_target));

unsigned const open_image_object_count = 78;

}
