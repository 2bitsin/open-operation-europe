// The staff's "Enemy Conditions" advice, and the services every advice check calls.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <span>

uint16_t opeu_advice_enemy_mines_or_bombardment(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_division_unfit(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_air_operations(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_attack_plans(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_objective_intent(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_recon_activity(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_plentiful_materials(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_unit_activity(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_commander_location(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_enemy_need_recon(Game& game, std::span<std::uint8_t const> far_text);
void     opeu_advice_enemy_conditions(Game& game, std::span<std::uint8_t const> far_text);
