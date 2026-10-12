#pragma once

#include <cstdint>

#include "data/image.hpp"

using opeu::game::FarCode;

#pragma pack(push, 1)
struct dg_3eba {
  FarCode idle_hook; /* 3eba: far routine called while waiting for a key */
  uint8_t z_3ebe[12];
  int16_t x;                /* 3eca: text cursor column */
  int16_t y;                /* 3ecc: text cursor row */
  int16_t left;             /* 3ece: window origin and extent */
  int16_t right;            /* 3ed0 */
  int16_t top;              /* 3ed2 */
  int16_t bottom;           /* 3ed4 */
  int16_t height;           /* 3ed6: line height */
  int16_t scroll;           /* 3ed8 */
  uint8_t paper;            /* 3eda */
  uint8_t ink;              /* 3edb */
  int16_t background;       /* 3edc */
  int16_t saved_x;          /* 3ede: context saved by @opeu_context_save */
  int16_t saved_y;          /* 3ee0 */
  int16_t saved_left;       /* 3ee2 */
  uint8_t saved_paper;      /* 3ee4 */
  uint8_t saved_ink;        /* 3ee5 */
  uint8_t saved_background; /* 3ee6 */
  uint8_t lead;             /* 3ee7 */
  int16_t state;            /* 3ee8: escape-sequence parser state */
  int16_t format_output;    /* 3eea: formatter output pointer */
};
#pragma pack(pop)

extern struct dg_3eba opeu_text_vars;
