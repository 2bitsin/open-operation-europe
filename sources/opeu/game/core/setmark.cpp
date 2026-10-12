// The marker on the setup screen: shown at one of the setup points over a saved patch of screen, hidden by
// restoring the patch.
#include "types.hpp"
#include "core/setmark.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "data-segment.hpp"
#include "common/memory.hpp"

using namespace opeu::game;

namespace {

// The services the marker calls through its event.
enum MarkerService : uint16_t {
  SAVE_AREA  = 0, // bitmap save (x, y, width, height, segment, offset)
  DRAW_IMAGE = 1, // far blit (x, y, width, height, mode, segment, offset)
  RESTORE_AREA =
      2, // bitmap restore (x, y, width, height, mode, segment, offset)
};

// The far buffer of the marker: the saved screen patch at offset 0, the marker image at 0x200.
constexpr uint16_t MARKER_SEGMENT{ 0x35e4 };
constexpr uint16_t SAVED_AREA{ 0x0000 };
constexpr uint16_t MARKER_IMAGE{ 0x0200 };

// The marker's size: 4 bytes (32 pixels) wide, 32 rows high.
constexpr uint16_t MARKER_WIDTH{ 4 };
constexpr uint16_t MARKER_HEIGHT{ 32 };

// Points gfx_source at `offset` in the marker buffer.
void PointSource(DataSegment& data, uint16_t offset) {
  data.gfx_source[0] = offset;
  data.gfx_source[1] = MARKER_SEGMENT;
}

} // namespace

void opeu_setup_marker_show(uint8_t ds[65536], HostEvent event, uint8_t index) {
  auto&          data{ Data(ds) };
  uint16_t const x{ data.setup_marker_points[index * 2] };
  uint16_t const y{ data.setup_marker_points[index * 2 + 1] };
  PointSource(data, SAVED_AREA);
  Call(ds, event, SAVE_AREA, x, y, MARKER_WIDTH, MARKER_HEIGHT,
       data.gfx_source[1], data.gfx_source[0]);
  PointSource(data, MARKER_IMAGE);
  Call(ds, event, DRAW_IMAGE, x, y, MARKER_WIDTH, MARKER_HEIGHT, 4,
       data.gfx_source[1], data.gfx_source[0]);
}

void opeu_setup_marker_hide(uint8_t ds[65536], HostEvent event, uint8_t index) {
  auto&          data{ Data(ds) };
  uint16_t const x{ data.setup_marker_points[index * 2] };
  uint16_t const y{ data.setup_marker_points[index * 2 + 1] };
  PointSource(data, SAVED_AREA);
  Call(ds, event, RESTORE_AREA, x, y, MARKER_WIDTH, MARKER_HEIGHT, 0,
       data.gfx_source[1], data.gfx_source[0]);
}

uint16_t svc_setmark(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  (void)ds;
  switch (id) {
    case SAVE_AREA:
      opeu_bitmap_save(Arg(args, count, 0), Arg(args, count, 1),
                       Arg(args, count, 2), Arg(args, count, 3),
                       port_far(Arg(args, count, 4), Arg(args, count, 5)));
      break;
    case DRAW_IMAGE:
      opeu_blit_far(Arg(args, count, 0), Arg(args, count, 1),
                    Arg(args, count, 2), Arg(args, count, 3),
                    Arg(args, count, 4),
                    port_far(Arg(args, count, 5), Arg(args, count, 6)));
      break;
    case RESTORE_AREA:
      opeu_bitmap_restore(Arg(args, count, 0), Arg(args, count, 1),
                          Arg(args, count, 2), Arg(args, count, 3),
                          Arg(args, count, 4),
                          port_far(Arg(args, count, 5), Arg(args, count, 6)));
      break;
    default: port_unhandled("setmark", id, count);
  }
  return 0;
}
