/* Address-keyed handlers of the native services. The game reaches these
 * library and C-runtime routines by their routine address (battle services,
 * far code pointers in the data); each handler converts the argument words
 * of port.hpp's convention to the service's parameters. The handlers of the
 * game's own routines live in their modules. */
#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

auto port_routine_getch(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  (void)args;
  (void)count;
  return (uint16_t)port_getch();
}

auto port_routine_opeu_gfx_blit(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  return (uint16_t)opeu_gfx_blit(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
}

auto port_routine_opeu_isqrt(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  return opeu_isqrt(Arg(args, count, 0));
}

auto port_routine_opeu_screen_copy(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  opeu_screen_copy(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5));
  return 0;
}

auto port_routine_opeu_turn_input_b(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  (void)args;
  (void)count;
  return opeu_turn_input_b();
}

auto port_routine_opeu_ui_line(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  opeu_ui_line(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
  return 0;
}

auto port_routine_opeu_vga_clear(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  (void)args;
  (void)count;
  return (uint16_t)opeu_vga_clear();
}

auto port_routine_opeu_vram_draw(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_draw(game.State().Bytes(Arg(args, count, 0)).data(), port_far(Arg(args, count, 1), Arg(args, count, 2)));
  return 0;
}

auto port_routine_opeu_vram_draw_near(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_draw_near(game.State().Bytes(Arg(args, count, 0)).data(), game.State().Bytes(Arg(args, count, 1)).data());
  return 0;
}

auto port_routine_opeu_vram_glyph(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_glyph(game.State().Bytes(Arg(args, count, 0)).data(), game.State().Bytes(Arg(args, count, 1)).data(), Arg(args, count, 2), Arg(args, count, 3));
  return 0;
}

auto port_routine_opeu_vram_glyph_color(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_glyph_color(game.State().Bytes(Arg(args, count, 0)).data(), game.State().Bytes(Arg(args, count, 1)).data(), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
  return 0;
}

auto port_routine_opeu_vram_load(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_load(game.State().Bytes(Arg(args, count, 0)).data(), port_far(Arg(args, count, 1), Arg(args, count, 2)));
  return 0;
}

auto port_routine_opeu_vram_restore(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_restore(Arg(args, count, 0), Arg(args, count, 1), game.State().Bytes(Arg(args, count, 2)).data());
  return 0;
}

auto port_routine_opeu_vram_restore_odd(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_restore_odd(Arg(args, count, 0), Arg(args, count, 1), game.State().Bytes(Arg(args, count, 2)).data());
  return 0;
}

auto port_routine_opeu_vram_save(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_save(Arg(args, count, 0), Arg(args, count, 1), game.State().Bytes(Arg(args, count, 2)).data());
  return 0;
}

auto port_routine_opeu_vram_save_odd(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_vram_save_odd(Arg(args, count, 0), Arg(args, count, 1), game.State().Bytes(Arg(args, count, 2)).data());
  return 0;
}
