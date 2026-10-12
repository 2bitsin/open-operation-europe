// The turn frame, panels, speaker dialogs, prompts and reports (core/dialogs).
#pragma once

#include "types.hpp"
#include "common/host.hpp"

void     opeu_panel_border(uint8_t* ds, const DialogHost* host, uint16_t which);
uint16_t opeu_message_backdrop(uint8_t* ds, const DialogHost* host);
void     opeu_turn_prepare(uint8_t* ds, const DialogHost* host);
void     opeu_turn_frame(uint8_t* ds, const DialogHost* host, uint8_t mode);
void     opeu_turn_active_begin(uint8_t* ds, const DialogHost* host);
void     opeu_context_begin(uint8_t* ds, const DialogHost* host);
void     opeu_info_panel_begin(uint8_t* ds, const DialogHost* host);
void     opeu_title_panel_begin(uint8_t* ds, const DialogHost* host);
void     opeu_message_panel_begin(uint8_t* ds, const DialogHost* host);
void     opeu_screen_reset(uint8_t* ds, const DialogHost* host);
void     opeu_army_title(uint8_t* ds, const DialogHost* host, uint16_t name);
void     opeu_daily_show(uint8_t* ds, const DialogHost* host, uint16_t id,
                         uint8_t side, uint32_t present);
uint16_t opeu_speaker_panel(uint8_t* ds, const DialogHost* host,
                            uint16_t person, uint16_t show);
void     opeu_text_feed(uint8_t* ds, const DialogHost* host, uint16_t text);
void opeu_control_message(uint8_t* ds, const DialogHost* host, uint16_t text,
                          uint16_t person);
uint16_t opeu_menu_prompt(uint8_t* ds, const DialogHost* host, uint16_t text,
                          uint16_t first, uint16_t last, uint16_t flag,
                          uint16_t menu);
uint16_t opeu_control_choose(uint8_t* ds, const DialogHost* host, uint16_t text,
                             uint16_t first, uint16_t last, uint16_t flag,
                             uint16_t person, uint16_t menu);
uint16_t opeu_delay_dialog(uint8_t* ds, const DialogHost* host, uint16_t text,
                           uint16_t last, uint16_t person, uint16_t menu);
uint16_t opeu_choose_dialog(uint8_t* ds, const DialogHost* host, uint16_t text,
                            uint16_t last, uint16_t person, uint16_t menu);
uint16_t opeu_confirm_prompt(uint8_t* ds, const DialogHost* host,
                             uint16_t text);
uint16_t opeu_confirm_message(uint8_t* ds, const DialogHost* host,
                              uint16_t text, uint16_t person);
uint16_t opeu_confirm_ok(uint8_t* ds, const DialogHost* host, uint16_t person);
uint16_t opeu_delay_confirm(uint8_t* ds, const DialogHost* host,
                            uint16_t person, uint8_t hours);
void     opeu_unit_report(uint8_t* ds, const DialogHost* host, uint16_t unit,
                          uint16_t message);
void     opeu_objective_report(uint8_t* ds, const DialogHost* host,
                               uint16_t objective, uint16_t message);
void opeu_unit_side_report(uint8_t* ds, const DialogHost* host, uint16_t unit,
                           uint16_t message);
void opeu_objective_side_report(uint8_t* ds, const DialogHost* host,
                                uint16_t objective, uint16_t message);
void opeu_element_type_panel(uint8_t* ds, const DialogHost* host,
                             uint16_t type);
void opeu_group_element_panel(uint8_t* ds, const DialogHost* host,
                              uint16_t group);
void opeu_element_panel(uint8_t* ds, const DialogHost* host, uint16_t element);

uint16_t svc_dialogs(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
