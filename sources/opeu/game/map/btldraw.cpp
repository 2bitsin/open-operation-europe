// Battle map drawing: the handler dispatch, a cell's screen position, the cursor, the cell frame and the battle's
// number.
#include "map/btldraw.hpp"
#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlsprt.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "io/soundcue.hpp"
#include "port.hpp"
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

void BlitHexMask(Game& game, uint16_t mask, uint16_t x, uint16_t y, uint16_t mode) {
  auto& data{ Data(game) };
  data.stream_ptr[0] = static_cast<uint16_t>(mask * HEX_MASK_BYTES);
  data.stream_ptr[1] = HEX_MASK_SEGMENT;
  game.Machine().Display().DrawShiftedBits({ x, y, HEX_WIDTH, HEX_HEIGHT }, { mode, FarBytes(game, data.stream_ptr[1], data.stream_ptr[0]) });
}

// The battle's sound effects, by the enumerator battle_handlers holds for the handler.
constexpr auto BATTLE_HANDLERS = DispatchRows<BattleHandler, auto (*)(Game&)->void>({
    { BattleHandler::EFFECT_0, opeu_sound_cue0    },
    { BattleHandler::EFFECT_1, opeu_sound_cue1    },
    { BattleHandler::EFFECT_2, opeu_sound_cue2    },
    { BattleHandler::EFFECT_3, opeu_sound_cue3    },
    { BattleHandler::EFFECT_4, opeu_sound_cue4    },
    { BattleHandler::EFFECT_5, opeu_sound_cue5    },
    { BattleHandler::EFFECT_6, opeu_sound_cue6    },
    { BattleHandler::EFFECT_7, opeu_sound_cue7    },
    { BattleHandler::EFFECT_8, opeu_control_begin },
    { BattleHandler::EFFECT_9, opeu_sound_cue9    },
});
} // namespace

// The battle handler `handler` of battle_handlers, unless it is NO_HANDLER.
void opeu_battle_dispatch(Game& game, uint8_t handler) {
  if (handler == NO_HANDLER) return;
  Dispatch(BATTLE_HANDLERS, Data(game).battle_handlers.at(handler), game);
}

uint16_t opeu_battle_cell_row(Game&, uint8_t x, uint8_t y) {
  return static_cast<uint16_t>((x + y + 2) * CELL_DY);
}

uint16_t opeu_battle_cell_column(Game&, uint8_t x, uint8_t y) {
  return static_cast<uint16_t>((x + 15) * CELL_DX - y * CELL_DX);
}

void opeu_battle_draw_cursor(Game& game, uint8_t x, uint8_t y) {
  auto const column{ opeu_battle_cell_column(game, x, y) };
  auto const row{ opeu_battle_cell_row(game, x, y) };
  BlitHexMask(game, CURSOR_MASK, column, row, CURSOR_MODE);
}

void opeu_battle_draw_frame(Game& game, uint8_t x, uint8_t y) {
  auto const column{ opeu_battle_cell_column(game, x, y) };
  auto const row{ opeu_battle_cell_row(game, x, y) };
  BlitHexMask(game, FRAME_OUTER_MASK, column, row, FRAME_OUTER_MODE);
  BlitHexMask(game, FRAME_INNER_MASK, column, row, FRAME_INNER_MODE);
}

// The number in the colour of the side holding the battle's objective.
void opeu_battle_draw_number(Game& game, uint16_t value) {
  auto& data{ Data(game) };
  if (data.battle_numbers_shown != 1) return;
  auto const colour{ opeu_objective_side(game, opeu_find_objective_at(game, data.battle_x, data.battle_y)) };
  auto&      display{ game.Machine().Display() };
  for (uint16_t x{ NUMBER_RIGHT };; x = static_cast<uint16_t>(x - DIGIT_WIDTH)) {
    auto const glyph{ game.State().Bytes(static_cast<uint16_t>(Near(data, data.battle_digits) + value % 10 * DIGIT_WIDTH), services::CELL_GLYPH_LINES).first<services::CELL_GLYPH_LINES>() };
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x - 1), NUMBER_TOP });
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x + 1), NUMBER_BOTTOM });
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x - 1), NUMBER_BOTTOM });
    display.DrawCellGlyph(data.vram, glyph, { static_cast<uint16_t>(x + 1), NUMBER_TOP });
    display.DrawColouredCellGlyph(data.vram, glyph,
                                  {
                                      .at = { x, NUMBER_ROW },
                                        .colour = colour
    });
    if ((value /= 10) == 0) break;
  }
}
