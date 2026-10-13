// The engineering order handlers (mines, bridges, repairs) and the services the order handlers share.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_mine_skill(Game& game);
void     opeu_mines_laid_finish(Game& game);
uint16_t opeu_engineer_lay_mines(Game& game);
void     opeu_mines_removed_finish(Game& game);
uint16_t opeu_engineer_remove_mines(Game& game);
uint8_t  opeu_bridge_skill(Game& game);
void     opeu_bridge_built_finish(Game& game);
uint16_t opeu_engineer_build_bridge(Game& game);
uint16_t opeu_arms_repair(Game& game, uint16_t& budget, uint16_t body);
void     opeu_arms_repaired_finish(Game& game);
uint16_t opeu_engineer_repair_objective(Game& game);
uint16_t opeu_engineer_repair_unit(Game& game);

int svc_order_handler(Game& game, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result);
