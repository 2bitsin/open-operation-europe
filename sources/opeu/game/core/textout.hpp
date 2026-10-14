// The text console: key reading, the printf-style formatter, the console window and glyph output.
#pragma once

#include "common/events.hpp"
#include "common/host.hpp"
#include "common/text.hpp"
#include "types.hpp"

#include <cstdint>
#include <span>

uint8_t opeu_key_read(Game& game);
void    opeu_input_flush(Game& game);
void    opeu_ui_text(Game& game, Text text);
int16_t opeu_text_scroll_mode(Game& game, int16_t mode);
void    opeu_text_window(Game& game, int16_t left, int16_t right, int16_t top, int16_t bottom);
int16_t opeu_text_scroll_check(Game& game);
void    opeu_text_scroll_up(Game& game);
void    opeu_text_set_left(Game& game, int16_t left);
void    opeu_text_window_reset(Game& game);
void    opeu_text_clamp(Game& game);
void    opeu_army_position(Game& game, int16_t x, int16_t y);
void    opeu_text_set_x(Game& game, int16_t x);
void    opeu_text_set_y(Game& game, int16_t y);
void    opeu_text_color(Game& game, uint16_t color);
void    opeu_text_paper_set(Game& game, uint8_t paper);
void    opeu_context_save(Game& game);
void    opeu_context_restore(Game& game);
void    opeu_text_glyph(Game& game, int16_t x, int16_t y, int16_t attribute, uint16_t code);
void    opeu_text_newline(Game& game);
void    opeu_text_control(Game& game, uint8_t c);
uint8_t opeu_text_put(Game& game, uint8_t c);
void    opeu_request_format(Game& game, TextTarget destination, std::span<FormatArgument const> words);
void    opeu_cursor_print(Game& game, std::span<FormatArgument const> words);
void    opeu_text_put_range(Game& game, Text text, uint8_t start, uint8_t count);
void    opeu_text_erase_char(Game& game);
