// The engineering order handlers (mines, bridges, repairs) and the services the order handlers share.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_mine_skill(uint8_t* ds, HostEvent event, uint16_t frame);
void     opeu_mines_laid_finish(uint8_t* ds, HostEvent event);
uint16_t opeu_engineer_lay_mines(uint8_t* ds, HostEvent event, uint16_t frame);
void     opeu_mines_removed_finish(uint8_t* ds, HostEvent event);
uint16_t opeu_engineer_remove_mines(uint8_t* ds, HostEvent event, uint16_t frame);
uint8_t  opeu_bridge_skill(uint8_t* ds, HostEvent event, uint16_t frame);
void     opeu_bridge_built_finish(uint8_t* ds, HostEvent event);
uint16_t opeu_engineer_build_bridge(uint8_t* ds, HostEvent event, uint16_t frame);
uint16_t opeu_arms_repair(uint8_t* ds, uint16_t& budget, uint16_t body);
void     opeu_arms_repaired_finish(uint8_t* ds, HostEvent event);
uint16_t opeu_engineer_repair_objective(uint8_t* ds, HostEvent event, uint16_t frame);
uint16_t opeu_engineer_repair_unit(uint8_t* ds, HostEvent event, uint16_t frame);

uint16_t svc_engineer(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
int      svc_order_handler(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result);
