// The text console: key reading, the printf-style formatter, the console window and glyph output.
#pragma once

#include "types.hpp"
#include "common/host.hpp"

uint8_t opeu_key_read(uint8_t* ds, TextHost* host);
void    opeu_input_flush(uint8_t* ds, TextHost* host);
int16_t opeu_format_decimal(uint8_t* ds, TextHost* host, uint16_t* p);
int16_t opeu_put_unsigned(uint8_t* ds, TextHost* host, uint16_t* p, uint16_t v,
                          uint16_t base);
int16_t opeu_put_long(uint8_t* ds, TextHost* host, uint16_t* p, int32_t v,
                      int16_t base);
void    opeu_format(uint8_t* ds, TextHost* host, uint16_t args, TextOut out);
void    opeu_ui_text(uint8_t* ds, TextHost* host, uint16_t s);
int16_t opeu_text_scroll_mode(uint8_t* ds, TextHost* host, int16_t mode);
void opeu_text_window(uint8_t* ds, TextHost* host, int16_t left, int16_t right,
                      int16_t top, int16_t bottom);
int16_t opeu_text_scroll_check(uint8_t* ds, TextHost* host);
void    opeu_text_scroll_up(uint8_t* ds, TextHost* host);
int16_t opeu_text_get_left(uint8_t* ds, TextHost* host);
int16_t opeu_text_get_right(uint8_t* ds, TextHost* host);
void    opeu_text_set_left(uint8_t* ds, TextHost* host, int16_t left);
void    opeu_text_set_right(uint8_t* ds, TextHost* host, int16_t right);
void    opeu_text_columns(uint8_t* ds, TextHost* host, int16_t left,
                          int16_t right);
void    opeu_text_window_reset(uint8_t* ds, TextHost* host);
int16_t opeu_text_set_height(uint8_t* ds, TextHost* host, int16_t height);
void    opeu_text_height_reset(uint8_t* ds, TextHost* host);
void    opeu_text_clamp(uint8_t* ds, TextHost* host);
void    opeu_army_position(uint8_t* ds, TextHost* host, int16_t x, int16_t y);
void    opeu_text_set_x(uint8_t* ds, TextHost* host, int16_t x);
void    opeu_text_set_y(uint8_t* ds, TextHost* host, int16_t y);
void    opeu_text_color(uint8_t* ds, TextHost* host, uint16_t color);
void    opeu_text_paper_set(uint8_t* ds, TextHost* host, uint8_t paper);
int16_t opeu_text_get_x(uint8_t* ds, TextHost* host);
int16_t opeu_text_get_y(uint8_t* ds, TextHost* host);
void    opeu_context_save(uint8_t* ds, TextHost* host);
void    opeu_context_restore(uint8_t* ds, TextHost* host);
void    opeu_text_glyph(uint8_t* ds, TextHost* host, int16_t x, int16_t y,
                        int16_t attribute, uint16_t code);
void    opeu_text_newline(uint8_t* ds, TextHost* host);
void    opeu_text_control(uint8_t* ds, TextHost* host, uint8_t c);
uint8_t opeu_text_put(uint8_t* ds, TextHost* host, uint8_t c);
void    opeu_format_store(uint8_t* ds, TextHost* host, uint16_t s);
void    opeu_request_format(uint8_t* ds, TextHost* host, uint16_t frame);
void    opeu_cursor_print(uint8_t* ds, TextHost* host, uint16_t frame);
void opeu_text_put_range(uint8_t* ds, TextHost* host, uint16_t s, uint8_t start,
                         uint8_t count);
void opeu_text_erase_char(uint8_t* ds, TextHost* host);

// The services the console calls.
uint16_t svc_textout(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
void     svc_textout_unused(uint8_t* a0, void* a1, uint16_t a2);
