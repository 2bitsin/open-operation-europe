// The marker on the setup screen: shown at one of the setup points over a saved patch of screen, hidden by
// restoring the patch.
#include "core/setmark.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "common/memory.hpp"
#include "data-segment.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

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

void opeu_setup_marker_show(Game& game, uint8_t index) {
  auto&          data{ Data(game) };
  uint16_t const x{ data.setup_marker_points[index * 2] };
  uint16_t const y{ data.setup_marker_points[index * 2 + 1] };
  PointSource(data, SAVED_AREA);
  game.Machine().Display().SaveColumns({ x, y, MARKER_WIDTH, MARKER_HEIGHT }, FarBytes(game, data.gfx_source[1], data.gfx_source[0]));
  PointSource(data, MARKER_IMAGE);
  game.Machine().Display().DrawThreeByteColumns({ x, y, MARKER_WIDTH, MARKER_HEIGHT }, { 4, FarBytes(game, data.gfx_source[1], data.gfx_source[0]) });
}

void opeu_setup_marker_hide(Game& game, uint8_t index) {
  auto&          data{ Data(game) };
  uint16_t const x{ data.setup_marker_points[index * 2] };
  uint16_t const y{ data.setup_marker_points[index * 2 + 1] };
  PointSource(data, SAVED_AREA);
  game.Machine().Display().DrawColumns({ x, y, MARKER_WIDTH, MARKER_HEIGHT }, { 0, FarBytes(game, data.gfx_source[1], data.gfx_source[0]) });
}
