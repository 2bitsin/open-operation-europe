#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct dg_5184 {
  char    s_5184_startup_mode[60];
  uint8_t z_51c0[2];
  uint8_t b_51c2[319];
  char    s_5301[53];
  char    s_5336[54];
};
#pragma pack(pop)

extern struct dg_5184 opeu_startup_text;
