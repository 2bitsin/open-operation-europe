// UI primitives: picture streams, the portrait and face caches, frames, backdrops and palettes.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/host.hpp"
#include "types.hpp"

void     opeu_image_read_draw(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, int16_t handle);
void     opeu_rle_expand(Game& game, const UiHost* host, uint16_t count, opeu::game::FarPointer src, opeu::game::FarPointer dst);
void     opeu_image_draw_masked(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode);
uint8_t  opeu_stream_file_byte(Game& game, const UiHost* host);
void     opeu_stream_seek_block(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t block);
void     opeu_picture_file(Game& game, const UiHost* host, uint16_t x, uint16_t y, int16_t handle, uint16_t block);
void     opeu_picture_file_masked(Game& game, const UiHost* host, uint16_t x, uint16_t y, int16_t handle, uint16_t block);
uint8_t  opeu_stream_memory_byte(Game& game, const UiHost* host);
void     opeu_picture_memory_masked(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t segment, uint16_t offset);
void     opeu_stream_seek(Game& game, const UiHost* host, int32_t position, int16_t handle);
void     opeu_picture_file_at(Game& game, const UiHost* host, uint16_t x, uint16_t y, int16_t handle, int32_t position);
void     opeu_context_frame(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t width);
uint16_t opeu_portrait_load(Game& game, const UiHost* host, uint8_t key);
void     opeu_context_portrait(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t person);
int16_t  opeu_face_load(Game& game, const UiHost* host, uint8_t key);
void     opeu_far_copy(Game& game, const UiHost* host, uint16_t count, opeu::game::FarPointer src, opeu::game::FarPointer dst);
void     opeu_face_draw(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t mode, uint8_t key);
void     opeu_ui_outline(Game& game, const UiHost* host, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void     opeu_backdrop_fill_color(Game& game, const UiHost* host, uint8_t color);
uint16_t opeu_title_box(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t text);
void     opeu_palette_for_hour(Game& game, const UiHost* host);
void     opeu_palette_daylight(Game& game, const UiHost* host);
void     opeu_palette_title_screen(Game& game, const UiHost* host);
void     opeu_palette_map_screen(Game& game, const UiHost* host);
