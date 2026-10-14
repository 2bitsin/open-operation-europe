// The special-forces missions' shared helpers: officer rating and experience, and the mission reports.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

uint16_t   opeu_officer_rating(Game const& game, uint8_t side, uint8_t kind);
void       opeu_officer_gain(Game& game, uint8_t side, uint8_t kind);
void       opeu_mission_finish_present(Game& game);
void       opeu_mission_delay_present(Game& game);
void       opeu_mission_report_success(Game& game, uint8_t side, Text text, uint8_t x, uint8_t y);
void       opeu_mission_report_enemy(Game& game, uint8_t side, Text text, uint8_t x, uint8_t y);
void       opeu_mission_report_failure(Game& game, uint8_t side, Text text);
TextTarget opeu_mission_target_name(Game& game, uint8_t side, uint16_t unit, TextTarget destination);
void       opeu_mission_person_killed(Game& game, uint16_t person);
