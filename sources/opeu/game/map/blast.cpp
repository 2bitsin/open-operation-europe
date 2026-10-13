// The explosion at a map location, the blast erase and the blast pause, and the map segment's shared services.
//
// The blast centres the map on the location for the side's status view, plays sound cue 5 and animates: in the
// overview two 1 x 8 frames, in the tactical view three 3 x 24 sprites, each followed by a short wait. The erase is
// the far wrapper of the cell erase the damage reports use; the pause is the wait passed as a far callback to the
// daily notice.
#include "map/blast.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/memory.hpp"
#include "core/sideutil.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ranges>

using namespace opeu::game;

namespace {

// The animation in the sprite segment: overview frames, tactical images and their masks.
constexpr uint16_t OVERVIEW_FRAMES{ 0x0260 };
constexpr uint16_t OVERVIEW_FRAME_BYTES{ 24 };
constexpr uint8_t  OVERVIEW_FRAME_COUNT{ 2 };
constexpr uint16_t VIEW_IMAGES{ 0x1fd0 };
constexpr uint16_t VIEW_IMAGE_BYTES{ 72 };
constexpr uint16_t VIEW_MASKS{ 0x20a8 };
constexpr uint16_t VIEW_MASK_BYTES{ 216 };
constexpr uint8_t  VIEW_FRAME_COUNT{ 3 };
constexpr uint16_t FRAME_WAIT{ 2 };
constexpr uint16_t PAUSE_WAIT{ 30 };
constexpr uint8_t  NONE{ 255 };

} // namespace

void opeu_location_blast(Game& game, uint8_t x, uint8_t y, uint8_t side) {
  auto&      data{ Data(game) };
  auto const shown{ static_cast<uint8_t>(opeu_side_for_status(game, static_cast<uint8_t>(side))) };
  opeu_arrival_location(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(shown));
  opeu_sound_cue5(game);
  opeu_delay_units(game, static_cast<uint8_t>(FRAME_WAIT));
  if (data.overview_side != NONE) {
    auto const pixel{ OverviewPixel({ x, y }) };
    for (auto const i : std::views::iota(uint8_t{ 0 }, OVERVIEW_FRAME_COUNT)) {
      SetStream(data, SPRITE_SEGMENT, static_cast<uint16_t>(OVERVIEW_FRAMES + OVERVIEW_FRAME_BYTES * i));
      opeu_blit_far(static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8, 4,
                    port_far(static_cast<std::uint16_t>(data.stream_ptr[1]), static_cast<std::uint16_t>(data.stream_ptr[0])));
      opeu_delay_units(game, static_cast<uint8_t>(FRAME_WAIT));
    }
  } else {
    auto const pixel{ ViewPixel(data, { x, y }) };
    for (auto const i : std::views::iota(uint8_t{ 0 }, VIEW_FRAME_COUNT)) {
      SetStream(data, SPRITE_SEGMENT, static_cast<uint16_t>(VIEW_IMAGES + VIEW_IMAGE_BYTES * i));
      opeu_sprite_compose(static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 3, 24,
                          port_far(static_cast<std::uint16_t>(data.stream_ptr[1]), static_cast<std::uint16_t>(data.stream_ptr[0])),
                          port_far(SPRITE_SEGMENT, static_cast<uint16_t>(VIEW_MASKS + VIEW_MASK_BYTES * i)), port_far(static_cast<std::uint16_t>(data.bitmap_buffer), 0), 0);
      opeu_delay_units(game, static_cast<uint8_t>(FRAME_WAIT));
    }
  }
  opeu_effect_finish(game);
}

void opeu_blast_erase(Game& game, uint8_t x, uint8_t y) {
  opeu_map_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
}

void opeu_blast_pause(Game& game) {
  opeu_delay_units(game, static_cast<uint8_t>(PAUSE_WAIT));
}
