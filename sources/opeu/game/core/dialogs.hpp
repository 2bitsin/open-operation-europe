// The turn frame, panels, speaker dialogs, prompts and reports (core/dialogs).
#pragma once

#include "common/events.hpp"
#include "common/host.hpp"
#include "common/presenter.hpp"
#include "types.hpp"

void     opeu_panel_border(Game& game, uint16_t which);
uint16_t opeu_message_backdrop(Game& game);
void     opeu_turn_prepare(Game& game);
void     opeu_turn_frame(Game& game, uint8_t mode);
void     opeu_turn_active_begin(Game& game);
void     opeu_context_begin(Game& game);
void     opeu_info_panel_begin(Game& game);
void     opeu_title_panel_begin(Game& game);
void     opeu_message_panel_begin(Game& game);
void     opeu_screen_reset(Game& game);
void     opeu_army_title(Game& game, uint16_t name);
void     opeu_daily_show(Game& game, uint16_t id, uint8_t side, opeu::game::Presenter present);
uint16_t opeu_speaker_panel(Game& game, uint16_t person, uint16_t show);
void     opeu_text_feed(Game& game, uint16_t text);
void     opeu_control_message(Game& game, uint16_t text, uint16_t person);
uint16_t opeu_menu_prompt(Game& game, uint16_t text, uint16_t first, uint16_t last, uint16_t flag, uint16_t menu);
uint16_t opeu_control_choose(Game& game, uint16_t text, uint16_t first, uint16_t last, uint16_t flag, uint16_t person, uint16_t menu);
uint16_t opeu_delay_dialog(Game& game, uint16_t text, uint16_t last, uint16_t person, uint16_t menu);
uint16_t opeu_choose_dialog(Game& game, uint16_t text, uint16_t last, uint16_t person, uint16_t menu);
uint16_t opeu_confirm_prompt(Game& game, uint16_t text);
uint16_t opeu_confirm_message(Game& game, uint16_t text, uint16_t person);
uint16_t opeu_confirm_ok(Game& game, uint16_t person);
uint16_t opeu_delay_confirm(Game& game, uint16_t person, uint8_t hours);
void     opeu_unit_report(Game& game, uint16_t unit, uint16_t message);
void     opeu_objective_report(Game& game, uint16_t objective, uint16_t message);
void     opeu_unit_side_report(Game& game, uint16_t unit, uint16_t message);
void     opeu_objective_side_report(Game& game, uint16_t objective, uint16_t message);
void     opeu_element_type_panel(Game& game, uint16_t type);
void     opeu_group_element_panel(Game& game, uint16_t group);
void     opeu_element_panel(Game& game, uint16_t element);
