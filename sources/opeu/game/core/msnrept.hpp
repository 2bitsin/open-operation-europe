// The special-forces missions' shared helpers: officer rating and experience, and the mission reports.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_officer_rating(const uint8_t* ds, uint8_t side, uint8_t kind);
void     opeu_officer_gain(uint8_t* ds, uint8_t side, uint8_t kind);
void     opeu_mission_finish_present(uint8_t* ds, HostEvent event);
void     opeu_mission_delay_present(uint8_t* ds, HostEvent event);
void     opeu_mission_report_success(uint8_t* ds, HostEvent event, uint8_t side, uint16_t text, uint8_t x, uint8_t y);
void     opeu_mission_report_enemy(uint8_t* ds, HostEvent event, uint8_t side, uint16_t text, uint8_t x, uint8_t y);
void     opeu_mission_report_failure(uint8_t* ds, HostEvent event, uint8_t side, uint16_t text);
uint16_t opeu_mission_target_name(uint8_t* ds, HostEvent event, uint8_t side, uint16_t unit, uint16_t destination);
void     opeu_mission_person_killed(uint8_t* ds, HostEvent event, uint16_t person);

// The services of the mission routines (MissionService).
uint16_t svc_msnrept(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
