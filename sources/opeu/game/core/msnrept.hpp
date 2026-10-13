// The special-forces missions' shared helpers: officer rating and experience, and the mission reports.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_officer_rating(Game const& game, uint8_t side, uint8_t kind);
void     opeu_officer_gain(Game& game, uint8_t side, uint8_t kind);
void     opeu_mission_finish_present(Game& game, HostEvent event);
void     opeu_mission_delay_present(Game& game, HostEvent event);
void     opeu_mission_report_success(Game& game, HostEvent event, uint8_t side, uint16_t text, uint8_t x, uint8_t y);
void     opeu_mission_report_enemy(Game& game, HostEvent event, uint8_t side, uint16_t text, uint8_t x, uint8_t y);
void     opeu_mission_report_failure(Game& game, HostEvent event, uint8_t side, uint16_t text);
uint16_t opeu_mission_target_name(Game& game, HostEvent event, uint8_t side, uint16_t unit, uint16_t destination);
void     opeu_mission_person_killed(Game& game, HostEvent event, uint16_t person);

// The services of the mission routines (MissionService).
uint16_t svc_msnrept(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
