// Names of equipment types, items and elements, and map-location labels.
#include "units/labels.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

// The type's name; tank types get the tank suffix.
uint16_t opeu_type_name(Game& game, uint16_t type, uint16_t buffer) {
  auto&       data{ Data(game) };
  auto const& record{ Record<ElementType>(data, type) };
  opeu_string_copy(game, buffer, record.name);
  if (record.category == Bits(Category::TANKS)) opeu_string_append(game, buffer, Near(data, data.type_name_suffix));
  return buffer;
}

// The name of an item's type.
uint16_t opeu_item_name(Game& game, uint16_t item, uint16_t buffer) {
  auto& data{ Data(game) };
  auto& type{ data.element_types[Record<Item>(data, item).type] };
  return opeu_string_append(game, opeu_type_name(game, Near(data, type), buffer), Near(data, data.item_name_suffix));
}

// The name of an element's type (the element's type, quantity and damage read as an item).
uint16_t opeu_element_name(Game& game, uint16_t element, uint16_t buffer) {
  auto& data{ Data(game) };
  return opeu_item_name(game, Near(data, Record<Element>(data, element).type), buffer);
}

// The name of the objective at the cell, else its coordinates scaled to the map.
uint16_t opeu_location_name(Game& game, HostEvent event, uint8_t x, uint8_t y, uint16_t buffer) {
  auto& data{ Data(game) };
  if (auto const objective{ opeu_find_objective_at(game, x, y) }; objective != 0) {
    opeu_string_copy(game, buffer, objective);
  } else {
    std::uint16_t args[4]{ buffer, Near(data, data.location_name_format), static_cast<std::uint16_t>(x * data.map_scale), static_cast<std::uint16_t>(y * data.map_scale) };
    event(game, 2, args, 4);
  }
  return buffer;
}

// The services the module calls out to: opeu_location_name formats the coordinates through request_format (id 2).
uint16_t svc_labels(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case 2:  opeu_request_format(game, { args, count }); break;
  default: port_unhandled("labels", id, count);
  }
  return r;
}
