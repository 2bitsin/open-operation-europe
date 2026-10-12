/* Address-keyed handlers of the native services. The game reaches these
 * library and C-runtime routines by their routine address (battle services,
 * far code pointers in the data); each handler converts the argument words
 * of port.hpp's convention to the service's parameters. The handlers of the
 * game's own routines live in their modules. */
#include "port.hpp"
#include "services.hpp"

auto port_routine_getch(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  (void)args;
  (void)count;
  return (uint16_t)port_getch();
}

auto port_routine_opeu_gfx_blit(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return (uint16_t)opeu_gfx_blit(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
}

auto port_routine_opeu_isqrt(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_isqrt(Arg(args, count, 0));
}

auto port_routine_opeu_screen_copy(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  opeu_screen_copy(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5));
  return 0;
}

auto port_routine_opeu_turn_input_b(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  (void)args;
  (void)count;
  return opeu_turn_input_b();
}

auto port_routine_opeu_ui_line(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  opeu_ui_line(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
  return 0;
}

auto port_routine_opeu_vga_clear(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  (void)args;
  (void)count;
  return (uint16_t)opeu_vga_clear();
}

auto port_routine_opeu_vram_draw(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_draw(ds + Arg(args, count, 0), port_far(Arg(args, count, 1), Arg(args, count, 2)));
  return 0;
}

auto port_routine_opeu_vram_draw_near(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_draw_near(ds + Arg(args, count, 0), ds + Arg(args, count, 1));
  return 0;
}

auto port_routine_opeu_vram_glyph(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_glyph(ds + Arg(args, count, 0), ds + Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3));
  return 0;
}

auto port_routine_opeu_vram_glyph_color(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_glyph_color(ds + Arg(args, count, 0), ds + Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
  return 0;
}

auto port_routine_opeu_vram_load(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_load(ds + Arg(args, count, 0), port_far(Arg(args, count, 1), Arg(args, count, 2)));
  return 0;
}

auto port_routine_opeu_vram_restore(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_restore(Arg(args, count, 0), Arg(args, count, 1), ds + Arg(args, count, 2));
  return 0;
}

auto port_routine_opeu_vram_restore_odd(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_restore_odd(Arg(args, count, 0), Arg(args, count, 1), ds + Arg(args, count, 2));
  return 0;
}

auto port_routine_opeu_vram_save(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_save(Arg(args, count, 0), Arg(args, count, 1), ds + Arg(args, count, 2));
  return 0;
}

auto port_routine_opeu_vram_save_odd(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_save_odd(Arg(args, count, 0), Arg(args, count, 1), ds + Arg(args, count, 2));
  return 0;
}
