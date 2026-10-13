// The fixture's plan dispatcher (word, far and near helpers, a local that reads a word), the cell dispatcher that hands
// it its calls, and Calls whose service id is a conditional or a parameter.
#include "map/plan.hpp"
#include "common/memory.hpp"

#include "port.hpp"

#include <utility>

uint16_t svc_plan(uint8_t* ds, const char* module, uint16_t kind, const uint16_t* args, uint16_t count) {
  auto const w{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const far{ [&](unsigned segment, unsigned offset) { return port_far(w(segment), w(offset)); } };
  auto const near{ [&](unsigned i) { return PORT_DS(w(i)); } };
  auto const x{ static_cast<uint8_t>(Arg(args, count, 0)) };
  switch (static_cast<PlanService>(kind)) {
  case PlanService::DRAW_FAR:           return opeu_draw_far(w(0), w(1), far(2, 3));
  case PlanService::DRAW_NEAR:          opeu_draw_near(w(0), w(1), near(2)); break;
  case PlanService::COMPOSE:            opeu_compose(far(1, 0), far(3, 2), w(4)); break;
  case PlanService::HIGHLIGHT_VIEW:     opeu_highlight_view(ds, x); break;
  case PlanService::HIGHLIGHT_OVERVIEW: opeu_highlight_overview(ds, x); break;
  default:                              port_unhandled(module, kind, count);
  }
  return 0;
}

uint16_t svc_cell(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<CellService>(id)) {
  case CellService::FLASH: return svc_plan(ds, "cell", std::to_underlying(PlanService::DRAW_FAR), args, count);
  default:                 return svc_plan(ds, "cell", id, args, count);
  }
}

void opeu_plan_draw(std::uint8_t* ds, std::uint16_t x, std::uint16_t y, std::uint16_t segment, std::uint16_t buffer) {
  Call(ds, svc_cell, CellService::FLASH, x, y, segment, buffer);
  Call(ds, svc_cell, PlanService::DRAW_NEAR, x, y, buffer);
  Call(ds, svc_cell, PlanService::COMPOSE, buffer, segment, 0, segment, x);
}

void opeu_plan_choose(std::uint8_t* ds, std::uint8_t side, std::uint16_t column) {
  Call(ds, svc_cell, side == 0 ? PlanService::HIGHLIGHT_VIEW : PlanService::HIGHLIGHT_OVERVIEW, column);
  Call(ds, svc_cell, column & 7 ? PlanService::DRAW_FAR : PlanService::DRAW_NEAR, column, side, 0x100, 0x200);
  auto const drawn{ Call(ds, svc_cell, column & 7 ? PlanService::DRAW_FAR : PlanService::DRAW_NEAR, column, side, 0x100, 0x200) };
  if (drawn) Call(ds, svc_cell, column & 7 ? PlanService::DRAW_FAR : PlanService::DRAW_NEAR, column, side, 0x100, 0x200);
  Call(ds, svc_cell, side == 0 ? PlanService::HIGHLIGHT_VIEW : PlanService::HIGHLIGHT_OVERVIEW, opeu_next_column(ds));
}

void opeu_plan_pixel(std::uint8_t* ds, PlanService service, std::uint16_t x) {
  Call(ds, svc_cell, service, x);
}
