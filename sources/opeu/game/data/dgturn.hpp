#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct dg_8070 {
  char    t_8070[8];
  char    s_8078_turn_label_format[48];
  char    s_80a8[6];
  char    s_80ae[5];
  char    s_80b3[2];
  uint8_t z_80b5[3];
  char    s_80b8[48];
  char    s_80e8[46];
  uint8_t b_8116[44];
  char    s_8142[10];
  char    s_814c[31];
  char    s_816b[4];
  char    s_816f[4];
  char    s_8173[4];
  char    s_8177[4];
  char    s_817b[4];
  char    s_817f[9];
  char    s_8188[7];
  char    s_818f[12];
  char    s_819b[10];
  char    s_81a5[5];
  char    s_81aa[10];
  char    s_81b4[7];
  char    s_81bb[3];
  char    s_81be[2];
  char    s_81c0[12];
  char    s_81cc[43];
  char    s_81f7[27];
  char    s_8212[23];
  char    s_8229[31];
  char    s_8248[14];
  char    s_8256[15];
  char    s_8265[15];
  char    s_8274[21];
  char    s_8289[132];
  char    s_830d[49];
  char    s_833e[17];
  char    s_834f[9];
  char    s_8358[8];
  char    s_8360[16];
  char    s_8370[7];
  uint8_t z_8377;
  char    s_8378[22];
};
#pragma pack(pop)

extern struct dg_8070 opeu_turn_text;
