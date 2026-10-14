// Battle element icons: the icon of an element's type, whether the player sees it, its count and its drawing.
#include "map/btlicon.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <ranges>

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

uint8_t opeu_battle_element_icon(Game& game, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& type{ ElementTypeOf(data, element) };
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

uint16_t opeu_battle_element_visible(Game& game, uint16_t element) {
  auto const side{ opeu_battle_element_side(game, element) };
  return Data(game).side_status[side] == SIDE_PLAYED || opeu_battle_element_flag2(game, element);
}

void opeu_battle_draw_count(Game& game, uint16_t count, uint8_t, uint16_t side) {
  auto& data{ Data(game) };
  if (data.battle_numbers_shown != 1) return;
  auto& display{ game.Machine().Display() };
  for (uint16_t x{ COUNT_RIGHT };; x = static_cast<uint16_t>(x - DIGIT_WIDTH)) {
    auto const glyph{ game.State().Bytes(static_cast<uint16_t>(Near(data, data.battle_digits) + count % 10 * DIGIT_WIDTH), services::CELL_GLYPH_LINES).first<services::CELL_GLYPH_LINES>() };
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x - 1), COUNT_TOP });
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x + 1), COUNT_BOTTOM });
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x - 1), COUNT_BOTTOM });
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x + 1), COUNT_TOP });
    display.DrawColouredCellGlyph(data.vram, glyph,
                                  {
                                      .at = { x, COUNT_ROW },
                                        .colour = side
    });
    if ((count /= 10) == 0) break;
  }
}

uint16_t opeu_battle_code_low(Game&, uint8_t code) {
  return (code & BATTLE_TERRAIN_MASK) <= LAST_PLAIN_CODE;
}

// Side 0's icons face the other way: each row's two halves are swapped.
void opeu_battle_draw_icon(Game& game, uint16_t element, uint8_t side, uint8_t mode) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ICON_BUFFER_BYTES };
  auto const          buffer{ locals.At(0) };
  auto const          icon{ opeu_battle_element_icon(game, element) };
  auto const          offset{ static_cast<uint16_t>((icon * MODES_PER_ICON + mode) * ICON_STRIDE) };
  auto const          segment{ data.icon_segment };
  data.stream_ptr[1] = segment;
  data.stream_ptr[0] = offset;
  std::ranges::copy(FarBytes(game, segment, offset).first(ICON_BYTES), game.State().Bytes(buffer, ICON_BYTES).begin());
  if (side != 1)
    for (auto const row : std::views::iota(uint16_t{ 0 }, static_cast<uint16_t>(ICON_BYTES / ICON_ROW))) {
      auto const bytes{ game.State().Bytes(static_cast<uint16_t>(buffer + row * ICON_ROW), 2 * HALF_ROW) };
      std::ranges::swap_ranges(bytes.first(HALF_ROW), bytes.subspan(HALF_ROW));
    }
  game.Machine().Display().MaskCell(data.vram, FarBytes(game, data.icon_segment, offset + ICON_BYTES).first<services::CELL_MASK_BYTES>());
  game.Machine().Display().OverlayCell(data.vram, game.State().Bytes(buffer, SYSLIB_CELL_BYTES).first<SYSLIB_CELL_BYTES>());
}
