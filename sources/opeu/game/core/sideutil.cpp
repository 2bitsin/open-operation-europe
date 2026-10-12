// Side helpers: the other side, the side a status points to, the side colour, the side an equipment type serves.
#include "core/sideutil.hpp"
#include "common/events.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "core/textout.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum TextColor : uint8_t {
  SIDE1_COLOR = 5,
  SIDE0_COLOR = 6,
};

} // namespace

uint8_t opeu_opposite_side(uint8_t side) {
  return !side;
}

// The side itself when its status is 1, otherwise the other side.
uint8_t opeu_side_for_status(const uint8_t ds[65536], uint8_t side) {
  return Data(ds).side_status[side] == 1 ? side : opeu_opposite_side(side);
}

void opeu_select_side_color(uint8_t ds[65536], SideColorEvent event, uint8_t side) {
  event(ds, side ? SIDE1_COLOR : SIDE0_COLOR);
}

// 0 when the element type at near pointer record is fielded by side 0's nations, else 1.
uint8_t opeu_record_side(const uint8_t ds[65536], uint16_t record) {
  auto const& data{ Data(ds) };
  if (data.side_masks[0] & Record<ElementType>(data, record).sides) return 0;
  return 1;
}

// The colour service of opeu_select_side_color: sets the text colour.
void svc_sideutil(uint8_t* ds, uint16_t color) {
  opeu_text_color(ds, (TextHost*)port_textout_host(), color);
}
