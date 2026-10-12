// Where a record sits in its table: the side, division and index of a division, unit or element near pointer.
#include "types.hpp"
#include "core/recindex.hpp"
#include "data-segment.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The index of the record at near pointer offset in the table at base. The distance is a signed 16-bit difference,
// so a pointer below the table gives a negative index (truncated toward zero by the division).
template <typename T> auto RecordIndex(uint16_t offset, uint16_t base) -> int {
  return static_cast<int16_t>(static_cast<uint16_t>(offset - base)) /
         static_cast<int>(sizeof(T));
}

// The table shapes as signed numbers, so that a negative index keeps its sign through % and /.
constexpr int GROUPS{ GROUPS_PER_SIDE };
constexpr int UNITS{ UNITS_PER_GROUP };
constexpr int SIDE_UNITS{ UNITS_PER_SIDE };
constexpr int SIDE_ELEMENTS{ ELEMENTS_PER_SIDE };

} // namespace

uint8_t opeu_group_index(uint16_t group) {
  return RecordIndex<Group>(group, GROUPS_AT) % GROUPS;
}

uint8_t opeu_group_side(uint16_t group) {
  return RecordIndex<Group>(group, GROUPS_AT) / GROUPS;
}

uint8_t opeu_unit_index(uint16_t unit) {
  return RecordIndex<Unit>(unit, UNITS_AT) % UNITS;
}

uint8_t opeu_unit_group_index(uint16_t unit) {
  return (RecordIndex<Unit>(unit, UNITS_AT) / UNITS) % GROUPS;
}

uint8_t opeu_unit_side(uint16_t unit) {
  return RecordIndex<Unit>(unit, UNITS_AT) / SIDE_UNITS;
}

// Near pointer of the division the unit belongs to.
uint16_t opeu_unit_group(uint16_t unit) {
  return static_cast<uint16_t>(
      GROUPS_AT +
      (opeu_unit_side(unit) * GROUPS_PER_SIDE + opeu_unit_group_index(unit)) *
          sizeof(Group));
}

uint8_t opeu_element_side(uint16_t element) {
  return RecordIndex<Element>(element, ELEMENTS_AT) / SIDE_ELEMENTS;
}

auto port_routine_opeu_unit_group_index(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_unit_group_index(Arg(args, count, 0));
}

auto port_routine_opeu_unit_side(std::uint8_t* ds, std::uint16_t const* args,
                                 unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_unit_side(Arg(args, count, 0));
}
