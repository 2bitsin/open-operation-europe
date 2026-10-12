// Battle map drawing: the handler dispatch, a cell's screen position, the cursor, the cell frame and the battle's
// number.
#include "types.hpp"
#include "map/btldraw.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/btlsprt.hpp"
#include "map/query.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NO_HANDLER{ 0xff };
// The hex masks: 40 x 26 pixels, 130 bytes each.
constexpr uint16_t HEX_MASK_SEGMENT{ 0x352d };
constexpr uint16_t HEX_MASK_BYTES{ 130 };
constexpr uint16_t HEX_WIDTH{ 40 };
constexpr uint16_t HEX_HEIGHT{ 26 };
constexpr uint16_t CELL_DX{ 20 };
constexpr uint16_t CELL_DY{ 13 };
// The hex masks and the blit modes they are drawn with.
constexpr uint16_t CURSOR_MASK{ 0 };
constexpr uint16_t CURSOR_MODE{ 0x104 };
constexpr uint16_t FRAME_OUTER_MASK{ 2 };
constexpr uint16_t FRAME_OUTER_MODE{ 0x20f };
constexpr uint16_t FRAME_INNER_MASK{ 1 };
constexpr uint16_t FRAME_INNER_MODE{ 0x307 };
// The number's digits from the right, with the outline offsets around each.
constexpr uint16_t NUMBER_RIGHT{ 23 };
constexpr uint16_t DIGIT_WIDTH{ 6 };
constexpr uint16_t NUMBER_TOP{ 6 };
constexpr uint16_t NUMBER_BOTTOM{ 8 };
constexpr uint16_t NUMBER_ROW{ 7 };

void BlitHexMask(uint8_t* ds, BattleEvent event, uint16_t mask, uint16_t x,
                 uint16_t y, uint16_t mode) {
  auto& data{ Data(ds) };
  data.stream_ptr[0] = static_cast<uint16_t>(mask * HEX_MASK_BYTES);
  data.stream_ptr[1] = HEX_MASK_SEGMENT;
  Call(ds, event, BattleRoutine::GFX_BLIT, x, y, HEX_WIDTH, HEX_HEIGHT, mode,
       data.stream_ptr[1], data.stream_ptr[0]);
}
} // namespace

// The battle handlers are far routines in a table of far pointers.
void opeu_battle_dispatch(uint8_t* ds, BattleEvent event, uint8_t handler) {
  if (handler == NO_HANDLER)
    return;
  auto&       data{ Data(ds) };
  auto const& routine{ Record<FarPointer>(
      data, static_cast<uint16_t>(Near(data, data.battle_handlers) +
                                  handler * sizeof(FarPointer))) };
  event(ds, static_cast<uint32_t>(routine.segment) * 16 + routine.offset,
        nullptr, 0);
}

uint16_t opeu_battle_cell_row(uint8_t*, BattleEvent, uint8_t x, uint8_t y) {
  return static_cast<uint16_t>((x + y + 2) * CELL_DY);
}

uint16_t opeu_battle_cell_column(uint8_t*, BattleEvent, uint8_t x, uint8_t y) {
  return static_cast<uint16_t>((x + 15) * CELL_DX - y * CELL_DX);
}

void opeu_battle_draw_cursor(uint8_t* ds, BattleEvent event, uint8_t x,
                             uint8_t y) {
  auto const column{ opeu_battle_cell_column(ds, event, x, y) };
  auto const row{ opeu_battle_cell_row(ds, event, x, y) };
  BlitHexMask(ds, event, CURSOR_MASK, column, row, CURSOR_MODE);
}

void opeu_battle_draw_frame(uint8_t* ds, BattleEvent event, uint8_t x,
                            uint8_t y) {
  auto const column{ opeu_battle_cell_column(ds, event, x, y) };
  auto const row{ opeu_battle_cell_row(ds, event, x, y) };
  BlitHexMask(ds, event, FRAME_OUTER_MASK, column, row, FRAME_OUTER_MODE);
  BlitHexMask(ds, event, FRAME_INNER_MASK, column, row, FRAME_INNER_MODE);
}

// The number in the colour of the side holding the battle's objective.
void opeu_battle_draw_number(uint8_t* ds, BattleEvent event, uint16_t value) {
  auto& data{ Data(ds) };
  if (data.battle_numbers_shown != 1)
    return;
  auto const colour{ opeu_objective_side(
      ds, opeu_find_objective_at(ds, data.battle_x, data.battle_y)) };
  auto const vram{ Near(data, data.vram) };
  for (uint16_t x{ NUMBER_RIGHT };;
       x = static_cast<uint16_t>(x - DIGIT_WIDTH)) {
    auto const glyph{ static_cast<uint16_t>(Near(data, data.battle_digits) +
                                            value % 10 * DIGIT_WIDTH) };
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x - 1, NUMBER_TOP);
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x + 1,
         NUMBER_BOTTOM);
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x - 1,
         NUMBER_BOTTOM);
    Call(ds, event, BattleRoutine::VRAM_GLYPH, vram, glyph, x + 1, NUMBER_TOP);
    Call(ds, event, BattleRoutine::VRAM_GLYPH_COLOR, vram, glyph, x, NUMBER_ROW,
         colour);
    if ((value /= 10) == 0)
      break;
  }
}

uint16_t svc_btldraw(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_draw_cursor(std::uint8_t*        ds,
                                          std::uint16_t const* args,
                                          unsigned count) -> std::uint32_t {
  opeu_battle_draw_cursor(ds, svc_btldraw, (uint8_t)Arg(args, count, 0),
                          (uint8_t)Arg(args, count, 1));
  return 0;
}

auto port_routine_opeu_battle_draw_frame(std::uint8_t*        ds,
                                         std::uint16_t const* args,
                                         unsigned count) -> std::uint32_t {
  opeu_battle_draw_frame(ds, svc_btldraw, (uint8_t)Arg(args, count, 0),
                         (uint8_t)Arg(args, count, 1));
  return 0;
}
