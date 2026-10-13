// Loss of a battle unit's headquarters element.
//
// The loss records the unit's side and division; when an element carried the headquarters, the new one is picked by
// the player through the element list (when the side has battle command and is player controlled) or automatically
// as the strongest element (non-combat types count a tenth; map/btldepty uses it too), and every element loses 2..4
// morale. A selected unit then risks its officers like map/btldepty's commander loss, but each hit is announced: a
// fatal hit marks the army commander lost (or replaces another officer through the deputy initializer) and routs
// the side; an injury shows the replacement's message and restores the record.
#include "map/hqloss.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/memory.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The DS reserves of the buffers handed to services, and their offsets in them.
constexpr uint16_t PICK_NAME_BYTES{ 0x22 };
constexpr uint16_t AUTO_RESERVE{ 0xae };
constexpr uint16_t AUTO_LABEL{ 0 };
constexpr uint16_t AUTO_TEXT{ 0x22 };
constexpr uint16_t AUTO_NAME{ 0x86 };
constexpr uint16_t DRAW_TEXT_BYTES{ 0x66 };
constexpr uint8_t  FIRST_DRAW{ 7 };
constexpr uint8_t  REDRAW{ 6 };
constexpr uint16_t MESSAGE_MODE{ 2 };
constexpr uint16_t HIT_WAIT{ 20 };
constexpr uint16_t SHOWN{ 1 }; // presentation_mode while battles are shown

// A non-combat element's strength counts a tenth.
[[nodiscard]] auto Strength(DataSegment const& data, uint16_t element) -> uint16_t {
  auto const& record{ Record<Element>(data, element) };
  auto        value{ record.quantity };
  if (!data.element_types[record.type].category) value /= 10;
  return value;
}

// One officer's messages: the format, then the fatal and the injury text.
struct OfficerTexts {
  uint16_t format;
  uint16_t killed;
  uint16_t injured;
};

// One officer draw: under 10 in 100 the officer (if any, and not already lost) is hit unless the staff trait saves
// him half the time; under 5 the hit is fatal. kind 0 is the division's general, otherwise the side's staff post.
void OfficerDraw(Game& game, BattleEvent event, uint8_t kind, OfficerTexts const& texts) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ DRAW_TEXT_BYTES };
  auto const          text{ locals.At(0) };
  auto const          side{ data.battle_active_side };
  auto const          draw{ static_cast<uint8_t>(Call(game, event, BattleRoutine::RAND_BOUNDED, 100)) };
  if (draw >= 10) return;
  auto const record{ kind ? opeu_find_side_record(game, side, kind) : opeu_find_group_record(game, side, data.battle_active_group) };
  if (!record || Record<Person>(data, record).role & Bits(PersonRole::DEPUTY)) return;
  if (Record<Person>(data, record).role & Bits(PersonRole::STAFF_TRAIT) && Call(game, event, BattleRoutine::RAND_BOUNDED, 100) < 50) return;
  auto const fatal{ draw < 5 };
  auto const outcome{ fatal ? texts.killed : texts.injured };
  if (kind)
    Call(game, event, BattleRoutine::REQUEST_FORMAT, text, texts.format, SideColorCode(side), data.side_names[side], record, outcome);
  else
    Call(game, event, BattleRoutine::REQUEST_FORMAT, text, texts.format, SideColorCode(side), data.side_names[side], data.battle_active_group + 1, record, outcome);
  opeu_battle_officer_hit(game, event, record, text, fatal);
}
} // namespace

void opeu_battle_pick_hq(Game& game, BattleEvent event) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ PICK_NAME_BYTES };
  auto const          name{ locals.At(0) };
  auto const          side{ data.battle_active_side };
  auto const          panels{ [&] {
    Call(game, event, BattleRoutine::TEXT_PAPER_SET, 0);
    Call(game, event, BattleRoutine::BATTLE_PANEL_SIDE1);
    Call(game, event, BattleRoutine::BATTLE_PANEL_SIDE0);
  } };
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_PANEL, side, 0);
  Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.hq_fmt_side), SideColorCode(side));
  if (auto const unit{ data.battle_lost_unit }; unit != Near(data, data.garrison_unit)) {
    auto const unit_name{ Call(game, event, BattleRoutine::UNIT_NAME, name, unit) };
    Call(game, event, BattleRoutine::UI_TEXT, unit_name);
  } else {
    Call(game, event, BattleRoutine::UI_TEXT, Near(data, data.hq_city_garrison));
  }
  Call(game, event, BattleRoutine::UI_TEXT, Near(data, data.hq_new_title));
  data.deploy_window.rows = static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_ELEMENT_COUNT, data.battle_lost_unit) + 1);
  {
    uint8_t row{ 0 };
    for (auto it{ Elements(data, Record<Unit>(data, data.battle_lost_unit)).begin() }; it != std::default_sentinel; ++it, ++row) data.ai_element_list[row] = it.Near();
  }
  data.deploy_row = 0;
  auto draw{ FIRST_DRAW };
  for (;; draw = REDRAW) {
    data.list_row = data.deploy_row;
    if (!Call(game, event, BattleRoutine::LIST_DIALOG, 1, draw, Near(data, data.deploy_window))) continue;
    auto element{ Record<Unit>(data, data.battle_lost_unit).first };
    data.deploy_row = data.list_row;
    for (uint8_t row{ 0 }; row < data.list_row; ++row) element = Record<Element>(data, element).next;
    Call(game, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, 0);
    if (Call(game, event, BattleRoutine::PROMPT_YES_NO, Near(data, data.hq_new_prompt))) {
      Record<Element>(data, element).flags |= Bits(ElementFlag::SELECTED);
      panels();
      return;
    }
    panels();
  }
}

void opeu_battle_auto_hq(Game& game, BattleEvent event) {
  auto&    data{ Data(game) };
  uint16_t best{ 0 };
  for (auto it{ Elements(data, Record<Unit>(data, data.battle_lost_unit)).begin() }; it != std::default_sentinel; ++it)
    if (!best || Strength(data, best) < Strength(data, it.Near())) best = it.Near();
  Record<Element>(data, best).flags |= Bits(ElementFlag::SELECTED);
  auto const unit{ data.battle_lost_unit };
  if (data.presentation_mode != SHOWN || unit == Near(data, data.garrison_unit)) return;
  auto const side{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_UNIT_SIDE, unit)) };
  if (data.side_status[side] != SIDE_PLAYED) return;
  ServiceLocals const locals{ AUTO_RESERVE };
  auto const          label{ Call(game, event, BattleRoutine::ELEMENT_NAME, locals.At(AUTO_LABEL), best) };
  auto const          name{ Call(game, event, BattleRoutine::UNIT_NAME, locals.At(AUTO_NAME), data.battle_lost_unit) };
  Call(game, event, BattleRoutine::REQUEST_FORMAT, locals.At(AUTO_TEXT), Near(data, data.hq_fmt_moved), SideColorCode(side), name, label);
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_WAIT, side, MESSAGE_MODE, locals.At(AUTO_TEXT));
}

void opeu_battle_officer_hit(Game& game, BattleEvent event, uint16_t record, uint16_t text, uint16_t killed) {
  auto&      data{ Data(game) };
  auto const side{ data.battle_active_side };
  data.battle_hq_intact[side] = 0;
  auto const message{ [&] { Call(game, event, BattleRoutine::BATTLE_COMMANDER_WAIT, data.battle_active_side, MESSAGE_MODE, text); } };
  if (killed) {
    if (opeu_find_side_record(game, side, std::to_underlying(StaffRole::ARMY_COMMANDER)) == record)
      Record<Person>(data, record).role |= Bits(PersonRole::DEPUTY);
    else
      Call(game, event, BattleRoutine::INITIALIZE_DEPUTY, record);
    message();
    data.battle_side_routed[data.battle_active_side] = 1;
  } else {
    Record<Person>(data, record).role |= Bits(PersonRole::POOR_HEALTH);
    auto const saved{ Record<Person>(data, record) };
    Call(game, event, BattleRoutine::INITIALIZE_DEPUTY, record);
    message();
    Record<Person>(data, record) = saved;
  }
  Call(game, event, BattleRoutine::DELAY_UNITS, HIT_WAIT);
}

void opeu_battle_officer_losses(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  OfficerDraw(game, event, 0, { Near(data, data.hq_fmt_division), Near(data, data.hq_div_killed), Near(data, data.hq_div_injured) });
  if (opeu_unit_group_index(opeu_find_side_selected_unit(game, data.battle_active_side)) != data.battle_active_group) return;
  OfficerDraw(game, event, std::to_underlying(StaffRole::ARMY_COMMANDER), { Near(data, data.hq_fmt_army), Near(data, data.hq_army_killed), Near(data, data.hq_army_injured) });
  OfficerDraw(game, event, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF), { Near(data, data.hq_fmt_intel), Near(data, data.hq_intel_killed), Near(data, data.hq_intel_injured) });
  OfficerDraw(game, event, std::to_underlying(StaffRole::OPERATIONS_CHIEF), { Near(data, data.hq_fmt_ops), Near(data, data.hq_ops_killed), Near(data, data.hq_ops_injured) });
}

uint16_t opeu_battle_side_commands(Game const& game) {
  auto const& data{ Data(game) };
  auto const  side{ data.battle_active_side };
  return data.battle_command[side] && data.side_control[side] == SIDE_PLAYED;
}

void opeu_battle_hq_shaken(Game& game, BattleEvent event, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    auto const loss{ Call(game, event, BattleRoutine::RAND_BOUNDED, 3) + 2 };
    opeu_sub_byte(static_cast<uint16_t>(loss), &element.morale);
  }
}

void opeu_battle_hq_lost(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  data.battle_active_side  = static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_UNIT_SIDE, data.battle_lost_unit));
  data.battle_active_group = opeu_unit_group_index(data.battle_lost_unit);
  if (data.battle_lost_element) {
    if (opeu_battle_side_commands(game))
      opeu_battle_pick_hq(game, event);
    else
      opeu_battle_auto_hq(game, event);
    opeu_battle_hq_shaken(game, event, data.battle_lost_unit);
  }
  if (Record<Unit>(data, data.battle_lost_unit).flags & Bits(UnitFlag::SELECTED)) opeu_battle_officer_losses(game, event);
}

uint16_t svc_hqloss(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_auto_hq(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_auto_hq(game, svc_hqloss);
  return 0;
}

auto port_routine_opeu_battle_hq_lost(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_hq_lost(game, svc_hqloss);
  return 0;
}
