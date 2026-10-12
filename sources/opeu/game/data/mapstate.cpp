/* Data segment DS:2012..27bd. These working arrays have explicit
 * initial file bytes, unlike the separately cleared runtime BSS region.
 */
#include "data/mapstate.hpp"

uint8_t  opeu_turn_order[64]        = { 0 };
uint16_t opeu_objective_links[42]   = { 0 };
uint16_t opeu_unit_links[8]         = { 0 };
uint8_t  opeu_map_cells[1600]       = { 0 };
int16_t  opeu_influence_map[10][10] = { { 0 } };
