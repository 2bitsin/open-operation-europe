#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_deploy_template_cell(Game& game, uint16_t x, uint16_t y, uint16_t element, uint8_t index);
void     opeu_deploy_by_template(Game& game, uint16_t x, uint16_t y, uint16_t element);
void     opeu_deploy_by_scan(Game& game, uint16_t x, uint16_t y, uint16_t element);
void     opeu_deploy_at_random(Game& game, uint16_t x, uint16_t y, uint16_t element);
void     opeu_deploy_put_at(Game& game, uint8_t x, uint8_t y, uint16_t element);
void     opeu_deploy_garrison(Game& game, uint16_t unit);
void     opeu_deploy_auto(Game& game, uint16_t unit);
