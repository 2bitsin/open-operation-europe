// The request menu's shared pieces: its service table and the helpers its choices use.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

// The services of the request menu (core/rqmenu, core/rqsupply, units/rqarms, units/rqreinf), called through their
// event. svc_request serves the ones core/rqcommon, core/rqmenu and core/rqsupply raise.
enum class RequestMenuService : uint16_t {
  PENDING =
      3, // opeu_request_pending(kind) -> nonzero when a request of the kind is filed
  CANCEL_PROMPT =
      4, // opeu_request_cancel_prompt(what) -> nonzero when the player cancels the filed request
  FILE        = 6,  // opeu_request_file(target, argument, kind)
  PANEL       = 9,  // the battalion request's panel (units/rqarms)
  MESSAGE     = 10, // opeu_control_message(text, person)
  LIST_DIALOG = 11, // the battalion type list (units/rqarms)
  TITLE       = 16, // opeu_army_title(text)
  DELAY_DIALOG =
      17, // opeu_delay_dialog(text, last, person, menu) -> 1-based choice, 0xffff when cancelled
  FORMAT  = 19, // request_format(destination, format, words...)
  CONFIRM = 20, // opeu_confirm_message(text, person) -> nonzero for yes
  FINISH  = 21, // opeu_pending_finish(value)
  SHOW    = 22, // opeu_daily_show(id, side, present offset, present segment)
  PICK = 23, // opeu_map_pick_location(x address, y address) -> 0 when cancelled
  TYPES    = 25, // the battalion types a side may request (units/rqarms)
  POSITION = 26, // opeu_army_position(x, y)
  PRINT    = 27, // print at the cursor (format, words...)
  MODE_IS  = 28, // opeu_request_mode_is(mode)
  HAS_CITY = 29, // opeu_side_has_city()
  FAR_CALL = 30, // calls the far routine (offset, segment)
};

void     opeu_request_no_more(uint8_t* ds, HostEvent event, uint16_t scratch,
                              uint16_t what);
uint16_t opeu_request_pending(const uint8_t* ds, uint8_t kind);
uint16_t opeu_request_mode_is(const uint8_t* ds, uint8_t mode);
uint16_t opeu_request_cancel_prompt(uint8_t* ds, HostEvent event,
                                    uint16_t scratch, uint16_t what);
void     opeu_request_finish(uint8_t* ds, HostEvent event);
void     opeu_request_file(uint8_t* ds, HostEvent event, uint16_t request);
uint16_t opeu_side_has_city(const uint8_t* ds);

// The request menu's services; module names the caller in diagnostics.
uint16_t svc_request(const char* module, uint8_t* ds, uint16_t id,
                     const uint16_t* args, uint16_t count);
uint16_t svc_rqcommon(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
