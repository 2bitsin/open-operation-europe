// The Settings command: options, saving, a new game and leaving to DOS.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void     opeu_settings_print(uint8_t* ds, HostEvent event);
void     opeu_settings_battles(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_settings_delay(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_settings_map(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_settings_sound(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_settings_pictures(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_save_rebase(uint8_t* ds);
void     opeu_save_unrebase(uint8_t* ds);
void     opeu_save_checksum(uint8_t* ds, uint8_t slot);
void     opeu_save_create(uint8_t* ds, HostEvent event);
uint16_t opeu_save_prompt_box(uint8_t* ds, HostEvent event, uint16_t x,
                              uint16_t y, uint16_t text);
void     opeu_save_game(uint8_t* ds, HostEvent event);
void     opeu_quit_to_dos(uint8_t* ds, HostEvent event);
void     opeu_settings_leave(uint8_t* ds, HostEvent event);
void     opeu_settings_new_game(uint8_t* ds, HostEvent event);
uint16_t opeu_settings_save_menu(uint8_t* ds, HostEvent event);
void     opeu_turn_action_10(uint8_t* ds, HostEvent event, uint16_t sp);

// The services the Settings routines call.
uint16_t svc_settings(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
