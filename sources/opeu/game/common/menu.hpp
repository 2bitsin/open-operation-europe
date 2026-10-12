// A button menu: an array of MenuItem records in the data segment, ended by a record whose label is 0.
#pragma once

#include <cstdint>

#pragma pack(push, 1)

// A button: width+1 eight-pixel tiles from column x, 24 pixels high from pixel row y, its label (a near pointer)
// printed in the attribute's low byte as colour. The selection is the id the menu returns; SAVE_BEHIND in it has the
// graphics the button covers saved before drawing and restored afterwards.
struct MenuItem {
  std::uint16_t x;
  std::uint16_t y;
  std::uint16_t width;
  std::uint16_t attribute;
  std::uint16_t label;
  std::uint16_t selection;
};

#pragma pack(pop)

static_assert(sizeof(MenuItem) == 12);

inline constexpr std::uint16_t SAVE_BEHIND{ 0x8000 };
inline constexpr std::uint16_t SELECTION_MASK{ 0x7fff };
