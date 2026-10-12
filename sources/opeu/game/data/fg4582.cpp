/* Far data segment 4582: two 3200-byte work areas, initially zero, which the game reaches through the segment
 * words at DS:c37c..c387: a queue of cell numbers (cell = row*40 + column, written from offset 0000) and a 40x40 word
 * grid at 0c80 that is first filled with ff bytes.
 */
#include "data/fg4582.hpp"
/* The module's far objects lie in declaration order, without padding, in one paragraph-aligned segment. */

uint16_t opeu_path_cell_queue[1600]  = { 0 };
uint16_t opeu_path_cost_grid[40][40] = { { 0 } };
