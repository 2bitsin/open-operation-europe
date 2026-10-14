// Names of equipment types, items and elements, and map-location labels.
#include "units/labels.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "common/text.hpp"

#include <array>

using namespace opeu::game;

// The type's name; tank types get the tank suffix.
TextTarget opeu_type_name(Game& game, uint16_t type, TextTarget buffer) {
  auto&       data{ Data(game) };
  auto const& record{ Record<ElementType>(data, type) };
  opeu_string_copy(game, buffer, record.name);
  if (record.category == Bits(Category::TANKS)) opeu_string_append(game, buffer, Near(data, data.type_name_suffix));
  return buffer;
}

// The name of an item's type.
TextTarget opeu_item_name(Game& game, uint16_t item, TextTarget buffer) {
  auto& data{ Data(game) };
  auto& type{ data.element_types[Record<Item>(data, item).type] };
  return opeu_string_append(game, opeu_type_name(game, Near(data, type), buffer), Near(data, data.item_name_suffix));
}

// The name of an element's type (the element's type, quantity and damage read as an item).
TextTarget opeu_element_name(Game& game, uint16_t element, TextTarget buffer) {
  auto& data{ Data(game) };
  return opeu_item_name(game, Near(data, Record<Element>(data, element).type), buffer);
}

// The name of the objective at the cell, else its coordinates scaled to the map.
TextTarget opeu_location_name(Game& game, uint8_t x, uint8_t y, TextTarget buffer) {
  auto& data{ Data(game) };
  if (auto const objective{ opeu_find_objective_at(game, x, y) }; objective != 0) {
    opeu_string_copy(game, buffer, objective);
  } else {
    opeu_request_format(game, buffer, std::array<FormatArgument, 3>{ Near(data, data.location_name_format), x * data.map_scale, y * data.map_scale });
  }
  return buffer;
}
