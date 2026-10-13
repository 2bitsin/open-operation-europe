// Battle element icons: the icon of an element's type, whether the player sees it, its count and its drawing.
#include "map/btlicon.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>

using namespace opeu::game;

namespace {
// The battle icon of an element type: 0 for none.
enum class Icon : uint8_t {
  NONE            = 0,
  INFANTRY_X10    = 1,
  INFANTRY_X5     = 2,
  LIGHT_TANKS     = 3,
  TANKS           = 4,
  SELF_PROPELLED  = 5,
  ARTILLERY       = 6,
  HEAVY_ARTILLERY = 7,
  ENGINEERS       = 8,
  REPAIR          = 9,
  SUPPLY          = 10,
};
constexpr uint16_t ICON_BYTES{ 520 };  // a 20 x 26 bitmap of the icon segment
constexpr uint16_t ICON_STRIDE{ 650 }; // a bitmap and its mask
// Bytes of a bitmap row: two mirrored halves of HALF_ROW.
constexpr uint16_t ICON_ROW{ 20 };
constexpr uint16_t HALF_ROW{ 5 };
constexpr uint8_t  MODES_PER_ICON{ 4 };
// The DS reserve of the bitmap buffer the far read fills and the draw reads.
constexpr uint16_t ICON_BUFFER_BYTES{ 0x214 };
// The count's digits from the right: the outline offsets around each digit and the digit's colours.
constexpr uint16_t COUNT_RIGHT{ 23 };
constexpr uint16_t DIGIT_WIDTH{ 6 };
constexpr uint16_t COUNT_TOP{ 13 };
constexpr uint16_t COUNT_BOTTOM{ 15 };
constexpr uint16_t COUNT_ROW{ 14 };
// The terrain codes below the hex-map extras.
constexpr uint8_t LAST_PLAIN_CODE{ 14 };
} // namespace

uint8_t opeu_battle_element_icon(uint8_t* ds, BattleEvent, uint16_t element) {
  auto&       data{ Data(ds) };
  auto const& type{ data.element_types[Record<Element>(data, element).type] };
  auto const  flags{ type.flags };
  Icon        icon{ Icon::NONE };
  switch (static_cast<Category>(type.category)) {
  case Category::INFANTRY_X10: icon = Icon::INFANTRY_X10; break;
  case Category::INFANTRY_X5:  icon = Icon::INFANTRY_X5; break;
  case Category::TANKS:        icon = flags & Bits(TypeFlag::LIGHT) ? Icon::LIGHT_TANKS : flags & Bits(TypeFlag::SELF_PROPELLED) ? Icon::SELF_PROPELLED : Icon::TANKS; break;
  case Category::ARTILLERY:    icon = flags & Bits(TypeFlag::HEAVY) ? Icon::HEAVY_ARTILLERY : Icon::ARTILLERY; break;
  case Category::ENGINEERS:    icon = Icon::ENGINEERS; break;
  case Category::REPAIR:       icon = Icon::REPAIR; break;
  case Category::SUPPLY:       icon = Icon::SUPPLY; break;
  default:                     break;
  }
  return std::to_underlying(icon);
}

uint16_t opeu_battle_element_visible(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto const side{ opeu_battle_element_side(ds, event, element) };
  return Data(ds).side_status[side] == SIDE_PLAYED || opeu_battle_element_flag2(ds, event, element);
}

void opeu_battle_draw_count(uint8_t* ds, BattleEvent event, uint16_t count, uint8_t, uint16_t side) {
  auto& data{ Data(ds) };
  if (data.battle_numbers_shown != 1) return;
  auto const vram{ Near(data, data.vram) };
  for (uint16_t x{ COUNT_RIGHT };; x = static_cast<uint16_t>(x - DIGIT_WIDTH)) {
    auto const glyph{ static_cast<uint16_t>(Near(data, data.battle_digits) + count % 10 * DIGIT_WIDTH) };
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x - 1, COUNT_TOP);
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x + 1, COUNT_BOTTOM);
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x - 1, COUNT_BOTTOM);
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x + 1, COUNT_TOP);
    Call(ds, event, BattleRoutine::VRAM_GLYPH_COLOR, vram, glyph, x, COUNT_ROW, side);
    if ((count /= 10) == 0) break;
  }
}

uint16_t opeu_battle_code_low(uint8_t*, BattleEvent, uint8_t code) {
  return (code & BATTLE_TERRAIN_MASK) <= LAST_PLAIN_CODE;
}

// Side 0's icons face the other way: each row's two halves are swapped.
void opeu_battle_draw_icon(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t side, uint8_t mode, uint16_t /*frame*/) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ ICON_BUFFER_BYTES };
  auto const          buffer{ locals.At(0) };
  auto const          icon{ opeu_battle_element_icon(ds, event, element) };
  auto const          offset{ static_cast<uint16_t>((icon * MODES_PER_ICON + mode) * ICON_STRIDE) };
  auto const          segment{ data.icon_segment };
  data.stream_ptr[1] = segment;
  data.stream_ptr[0] = offset;
  Call(ds, event, BattleRoutine::READ_FAR_BYTES, ICON_BYTES, buffer, offset, segment);
  if (side != 1)
    for (uint16_t row{ 0 }; row < ICON_BYTES; row += ICON_ROW) {
      auto* const left{ ds + static_cast<uint16_t>(buffer + row) };
      std::swap_ranges(left, left + HALF_ROW, left + HALF_ROW);
    }
  Call(ds, event, BattleRoutine::VRAM_LOAD, Near(data, data.vram), data.icon_segment, offset + ICON_BYTES);
  Call(ds, event, BattleRoutine::VRAM_DRAW_NEAR, Near(data, data.vram), buffer);
}

uint16_t svc_btlicon(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}
