// The text console: key reading, the printf-style formatter, the console window and glyph output.
#pragma once

#include "common/events.hpp"
#include "common/host.hpp"
#include "types.hpp"

#include <cstdint>
#include <span>

uint8_t opeu_key_read(Game& game, TextHost* host);
void    opeu_input_flush(Game& game, TextHost* host);
void    opeu_ui_text(Game& game, TextHost* host, uint16_t s);
int16_t opeu_text_scroll_mode(Game& game, TextHost* host, int16_t mode);
void    opeu_text_window(Game& game, TextHost* host, int16_t left, int16_t right, int16_t top, int16_t bottom);
int16_t opeu_text_scroll_check(Game& game, TextHost* host);
void    opeu_text_scroll_up(Game& game, TextHost* host);
int16_t opeu_text_get_left(Game& game, TextHost* host);
int16_t opeu_text_get_right(Game& game, TextHost* host);
void    opeu_text_set_left(Game& game, TextHost* host, int16_t left);
void    opeu_text_set_right(Game& game, TextHost* host, int16_t right);
void    opeu_text_columns(Game& game, TextHost* host, int16_t left, int16_t right);
void    opeu_text_window_reset(Game& game, TextHost* host);
int16_t opeu_text_set_height(Game& game, TextHost* host, int16_t height);
void    opeu_text_height_reset(Game& game, TextHost* host);
void    opeu_text_clamp(Game& game, TextHost* host);
void    opeu_army_position(Game& game, TextHost* host, int16_t x, int16_t y);
void    opeu_text_set_x(Game& game, TextHost* host, int16_t x);
void    opeu_text_set_y(Game& game, TextHost* host, int16_t y);
void    opeu_text_color(Game& game, TextHost* host, uint16_t color);
void    opeu_text_paper_set(Game& game, TextHost* host, uint8_t paper);
int16_t opeu_text_get_x(Game& game, TextHost* host);
int16_t opeu_text_get_y(Game& game, TextHost* host);
void    opeu_context_save(Game& game, TextHost* host);
void    opeu_context_restore(Game& game, TextHost* host);
void    opeu_text_glyph(Game& game, TextHost* host, int16_t x, int16_t y, int16_t attribute, uint16_t code);
void    opeu_text_newline(Game& game, TextHost* host);
void    opeu_text_control(Game& game, TextHost* host, uint8_t c);
uint8_t opeu_text_put(Game& game, TextHost* host, uint8_t c);
void    opeu_request_format(Game& game, std::span<std::uint16_t const> words);
void    opeu_cursor_print(Game& game, std::span<std::uint16_t const> words);
void    opeu_text_put_range(Game& game, TextHost* host, uint16_t s, uint8_t start, uint8_t count);
void    opeu_text_erase_char(Game& game, TextHost* host);

// The services the console calls.
uint16_t svc_textout(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
void     svc_textout_unused(uint8_t* a0, void* a1, uint16_t a2);
