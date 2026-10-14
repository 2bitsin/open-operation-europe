// The staff's "Regiment Status" advice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <span>

uint16_t opeu_advice_regiment_order_finishing(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_regiment_supply_shortage(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_division_commander_condition(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_regiment_damage_or_comms(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_regiment_fatigue_morale(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_regiment_nothing(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_regiment_status(Game& game, std::span<std::uint8_t const> far_text);
