// The staff's "Tide of War" advice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <span>

uint16_t opeu_advice_tide_isolation(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_staff_1ad2(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_tide_strength_balance(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_tide_territory_control(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_tide_air_superiority(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_tide_no_advances(Game& game, std::span<std::uint8_t const> far_text);
void     opeu_advice_tide_of_war(Game& game, std::span<std::uint8_t const> far_text);
