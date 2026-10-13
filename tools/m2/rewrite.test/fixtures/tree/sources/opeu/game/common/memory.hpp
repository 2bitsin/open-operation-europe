// The fixture's Call: the words go to the event as an array (the template's own call is not a site).
#pragma once

#include "port.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>

template <typename Event, typename Service, typename... Words>
inline auto Call(std::uint8_t* ds, Event event, Service service, Words... words) {
  if constexpr (std::is_enum_v<Service>) {
    return Call(ds, event, std::to_underlying(service), words...);
  } else {
    std::uint16_t args[]{ static_cast<std::uint16_t>(words)..., 0 };
    return event(ds, service, args, static_cast<std::uint16_t>(sizeof...(Words)));
  }
}
