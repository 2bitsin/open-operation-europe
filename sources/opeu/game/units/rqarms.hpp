// The battalion (arms) request of the request menu, and the services the menu's requests share.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

#include <cstdint>

// The services the battalion and regiment requests (units/rqarms, units/rqreinf) call through their event, dispatched
// by svc_units_request. Text, list and record arguments are DS offsets.
enum class RequestService : std::uint16_t {
  RANDOM        = 0, // (bound) -> value
  MIN16         = 1, // (a, b) -> min
  MAX16         = 2, // (a, b) -> max
  PENDING       = 3, // (kind) -> 0/1: a request of the kind is filed
  CANCEL_PROMPT = 4, // (text) -> 1 to cancel the filed request
  NO_MORE       = 5, // (text): there are no more of them
  FILE_REQUEST  = 6, // (target, argument, kind)
  FREE_UNITS =
      7, // (list, flag) -> count: collects (or with list 0 releases) the side's free units
  FREE_ELEMENTS   = 8,  // (list, flag) -> count: the same for the free elements
  CONTROL_PANEL   = 9,  // ()
  CONTROL_MESSAGE = 10, // (text, person)
  LIST_DIALOG     = 11, // (dialog, mode, height) -> 0 when cancelled
  UNIT_INDEX      = 12, // (unit) -> its slot in the division
  CATEGORY_0_TO_3 = 13, // (slot) -> 0/1
  CATEGORY_4_OR_5 = 14, // (slot) -> 0/1
  CATEGORY_6      = 15, // (slot) -> 0/1
  ARMY_TITLE      = 16, // (text)
  DELAY_DIALOG =
      17, // (text, last, person, menu) -> choice, 0xffff when cancelled
  CONFIRM_OK        = 18, // (person) -> 0/1
  REQUEST_FORMAT    = 19, // (destination, format, ...)
  CONFIRM_MESSAGE   = 20, // (text, person) -> 0/1
  MAP_PICK_LOCATION = 23, // (x address, y address) -> 0 when cancelled
  ELEMENT_TYPES =
      25, // (nation mask, category, list) -> count of the due types written to the list
};

void    opeu_request_arms(uint8_t* ds, HostEvent event, uint16_t scratch);
uint8_t opeu_rq_unit_selectable(void);

uint16_t svc_rqarms(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count);
uint16_t svc_units_request(const char* module, uint8_t* ds, uint16_t id,
                           const uint16_t* args, uint16_t count);
