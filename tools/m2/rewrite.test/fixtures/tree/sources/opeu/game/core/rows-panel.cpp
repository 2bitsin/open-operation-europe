// The rows service.
#include "core/rows-panel.hpp"

#include "port.hpp"

uint16_t svc_rows(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case ROWS_FIRST: return opeu_rows_first(ds, nullptr);
  default:         port_unhandled("rows", id, count);
  }
  return 0;
}
