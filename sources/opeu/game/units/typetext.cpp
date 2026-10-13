// Equipment type names, nation codes, chain length and the unit-slot class comparison.
#include "units/typetext.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include <iterator>

using namespace opeu::game;

// The name of an item's equipment type; tanks that are not self-propelled guns get " Tank" appended.
uint16_t opeu_item_type_name(Game& game, uint16_t item, uint16_t buffer) {
  auto&       data{ Data(game) };
  auto const& type{ data.element_types[Record<Item>(data, item).type] };
  opeu_string_copy(game, buffer, type.name);
  if (type.category == Bits(Category::TANKS) && !(type.flags & Bits(TypeFlag::SELF_PROPELLED))) opeu_string_append(game, buffer, Near(data, data.tank_suffix));
  return buffer;
}

// The three-letter code of the nation that fields an equipment type; the combined bits 12 (8|4) also print as USA.
uint16_t opeu_nation_code(Game& game, uint16_t type, uint16_t buffer) {
  auto&         data{ Data(game) };
  std::uint16_t code;
  switch (Record<ElementType>(data, type).sides) {
  case 1:  code = Near(data, data.nation_codes[0]); break;
  case 2:  code = Near(data, data.nation_codes[1]); break;
  case 4:  code = Near(data, data.nation_codes[2]); break;
  case 8:  code = Near(data, data.nation_codes[3]); break;
  case 16: code = Near(data, data.nation_codes[4]); break;
  case 32: code = Near(data, data.nation_codes[5]); break;
  case 12: code = Near(data, data.nation_codes[6]); break;
  default: code = Near(data, data.nation_code_other); break;
  }
  opeu_string_copy(game, buffer, code);
  return buffer;
}

// The number of elements chained after a list head (a record whose first word links the first element).
uint8_t opeu_list_length(Game& game, uint16_t list) {
  auto& data{ Data(game) };
  return static_cast<std::uint8_t>(std::ranges::distance(Elements(data, Record<std::uint16_t>(data, list))));
}

// Unit slots 0-5 form one class; slots 6 and 7 each form their own.
uint16_t opeu_same_unit_class(uint16_t first, uint16_t second) {
  auto const a{ opeu_unit_index(first) };
  auto const b{ opeu_unit_index(second) };
  if (opeu_category_0_to_3(a) || opeu_category_4_or_5(a)) {
    if (opeu_category_0_to_3(b) || opeu_category_4_or_5(b)) return 1;
  } else if (opeu_category_6(a))
    return opeu_category_6(b);
  else if (opeu_category_7(a))
    return opeu_category_7(b);
  return 0;
}
