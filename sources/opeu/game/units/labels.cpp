// Names of equipment types, items and elements, and map-location labels.
#include "types.hpp"
#include "units/labels.hpp"
#include "common/data-segment.hpp"
#include "core/strings.hpp"
#include "map/query.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

// The type's name; tank types get the tank suffix.
uint16_t opeu_type_name(uint8_t ds[65536], uint16_t type, uint16_t buffer) {
  auto&       data{ Data(ds) };
  auto const& record{ Record<ElementType>(data, type) };
  opeu_string_copy(ds, buffer, record.name);
  if (record.category == Bits(Category::TANKS))
    opeu_string_append(ds, buffer, Near(data, data.type_name_suffix));
  return buffer;
}

// The name of an item's type.
uint16_t opeu_item_name(uint8_t ds[65536], uint16_t item, uint16_t buffer) {
  auto& data{ Data(ds) };
  auto& type{ data.element_types[Record<Item>(data, item).type] };
  return opeu_string_append(ds, opeu_type_name(ds, Near(data, type), buffer),
                            Near(data, data.item_name_suffix));
}

// The name of an element's type (the element's type, quantity and damage read as an item).
uint16_t opeu_element_name(uint8_t ds[65536], uint16_t element,
                           uint16_t buffer) {
  auto& data{ Data(ds) };
  return opeu_item_name(ds, Near(data, Record<Element>(data, element).type),
                        buffer);
}

// The name of the objective at the cell, else its coordinates scaled to the map.
uint16_t opeu_location_name(uint8_t ds[65536], HostEvent event, uint8_t x,
                            uint8_t y, uint16_t buffer) {
  auto& data{ Data(ds) };
  if (auto const objective{ opeu_find_objective_at(ds, x, y) };
      objective != 0) {
    opeu_string_copy(ds, buffer, objective);
  } else {
    std::uint16_t args[4]{ buffer, Near(data, data.location_name_format),
                           static_cast<std::uint16_t>(x * data.map_scale),
                           static_cast<std::uint16_t>(y * data.map_scale) };
    event(ds, 2, args, 4);
  }
  return buffer;
}

// The services the module calls out to: opeu_location_name formats the coordinates through request_format (id 2).
uint16_t svc_labels(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count) {
  uint16_t r = 0;
  switch (id) {
    case 2:  r = lf_request_format(ds, args, count); break;
    default: port_unhandled("labels", id, count);
  }
  return r;
}

auto port_routine_opeu_element_name(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  return opeu_element_name(ds, Arg(args, count, 1), Arg(args, count, 0));
}
