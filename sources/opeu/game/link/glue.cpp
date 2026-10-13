// Platform glue of the game's services (declarations and rationale: link/glue.hpp): the random state, argument
// word placement, driver register blocks and file loading. No game logic.
#include "link/glue.hpp"

#include <cstdlib>
#include <cstring>

#include "data-segment.hpp"

#include "common/game.hpp"
#include "core/aiops1.hpp"
#include "core/dialogs.hpp"
#include "core/random.hpp"
#include "core/scenbrf.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/loadscen.hpp"
#include "io/resopen.hpp"
#include "link/link.hpp"
#include "map/cellpred.hpp"
#include "services.hpp"

namespace {
using opeu::game::Data;
using opeu::game::Near;
} // namespace

/* ---- C runtime random state ------------------------------------------------ *
 * rand_bounded: the generator keeps its state in a global; the saved game
 * keeps it in the data segment's random_state. */
uint16_t port_rand_bounded(Game& game, uint16_t bound) {
  auto& state{ Data(game).random_state };
  std::memcpy(&opeu_random_state, state.data(), 4);
  auto const r{ opeu_rand_bounded(bound) };
  std::memcpy(state.data(), &opeu_random_state, 4);
  return r;
}

/* ---- DS words ---------------------------------------------------------------- */

/* Host pointer of a DS word the callee writes (output arguments). */
uint16_t* port_ds_word_ptr(Game& game, uint16_t near) {
  return reinterpret_cast<uint16_t*>(game.State().Bytes(near, sizeof(uint16_t)).data());
}

/* ---- far code pointers ------------------------------------------------------- *
 * The data and the callers name the two cell predicates by their far code
 * address (segment:offset of the routine in the original layout). */

namespace {
constexpr std::uint32_t REQUEST_CELL_ALLOWED_AT{ 0x145cu * 16u + 0x1114u };
constexpr std::uint32_t ARRIVAL_CELL_ALLOWED_AT{ 0x145cu * 16u + 0x1152u };
} // namespace

PortCellPredicate c2_cell_predicate(uint16_t off, uint16_t seg) {
  uint32_t address = (uint32_t)seg * 16u + off;
  if (address == REQUEST_CELL_ALLOWED_AT) return opeu_request_cell_allowed;
  if (address == ARRIVAL_CELL_ALLOWED_AT) return opeu_arrival_cell_allowed;
  port_fatal("request_cells: predicate %04x:%04x is not a cell predicate", seg, off);
  return 0;
}

/* Daily presentation: daily_show(id, side, far presenter). The presenter is a (offset, segment) pair; DIALOGS calls it back through its present service, i.e. by routine identifier. */
void port_daily_show(Game& game, uint16_t id, uint16_t side, uint16_t off, uint16_t seg) {
  opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), id, (uint8_t)side, (uint32_t)seg << 16 | off);
}

uint32_t port_call_named(Game& game, const char* name, const uint16_t* args, unsigned count) {
  uint32_t address = port_routine_address(name);
  if (!address) port_fatal("no routine named %s", name);
  return port_call(game, address, args, count);
}

/* ---- driver register blocks ------------------------------------------------- */

/* INT 33h: int86 copies the output registers back and returns AX. */
uint16_t port_mouse_int33(uint16_t* w) {
  switch (w[0]) {
  case 0:
    w[0] = port_mouse_reset();
    w[1] = 2; /* BX: number of buttons */
    break;
  case 1:  port_mouse_show(); break;
  case 2:  port_mouse_hide(); break;
  case 3:  port_mouse_state(&w[1], &w[2], &w[3]); break;
  case 4:  port_mouse_set_position(w[2], w[3]); break;
  case 7:  port_mouse_x_range(w[2], w[3]); break;
  case 8:  port_mouse_y_range(w[2], w[3]); break;
  default: port_fatal("mouse: INT 33h function %04x is not issued by MAIN", w[0]);
  }
  w[6] = 0; /* cflag: int86 copies CF, clear */
  return w[0];
}

/* INT 66h: AX, BX, DX and DS are the driver's inputs and results; CX, SI,
 * DI come back unchanged; the carry flag is clear. */
uint16_t port_sound_int66(uint16_t* w, uint16_t* ds_seg) {
  port_fm_int66(&w[0], &w[1], &w[3], ds_seg);
  w[6] = 0;
  return w[0];
}

/* ---- keyboard prompts ----------------------------------------------------------- */

uint16_t port_prompt_cancel(Game& game, uint16_t text, uint16_t max, uint16_t items) {
  return opeu_prompt_value_cancel(game, text, max, items);
}

uint16_t port_prompt_required(Game& game, uint16_t text, uint16_t max, uint16_t items) {
  return opeu_prompt_value_required(game, text, max, items);
}

uint16_t port_prompt_yes_no(Game& game, uint16_t text) {
  return (uint16_t)opeu_prompt_yes_no(game, text);
}

/* ---- LOADSCEN ------------------------------------------------------------------- *
 * The scenario loader (io/loadscen) takes the contents of the two files the
 * data segment names (scenario_filename, map_filename): open them as
 * resources and hand over their whole contents. */

static uint8_t* read_resource(Game& game, uint16_t name, size_t* size) {
  int16_t  h   = opeu_open_resource(game, name);
  int32_t  end = h < 0 ? 0 : opeu_dos_seek(h, 0, 2);
  uint8_t* data;
  size_t   done = 0;
  if (end < 0) end = 0;
  data = (uint8_t*)std::malloc((size_t)end + 1);
  if (!data) port_fatal("scensel: out of memory");
  if (h >= 0) {
    opeu_dos_seek(h, 0, 0);
    while (done < (size_t)end) {
      size_t   chunk = (size_t)end - done;
      uint16_t n;
      if (chunk > 0x8000u) chunk = 0x8000u;
      n = (uint16_t)opeu_dos_read(h, data + done, (uint16_t)chunk); /* AX: unsigned count */
      if (n == 0 || n == 0xffffu) break;
      done += n;
    }
    opeu_dos_close(h);
  }
  *size = done;
  return data;
}

void port_load_scenario(Game& game, uint16_t scenario) {
  size_t   scedata_size, mapinf_size;
  auto&    data{ Data(game) };
  uint8_t* scedata = read_resource(game, Near(data, data.scenario_filename), &scedata_size);
  uint8_t* mapinf  = read_resource(game, Near(data, data.map_filename), &mapinf_size);
  /* The loader returns -1 (and leaves DS untouched) when a file is shorter
   * than the records it reads; the game has no result for that, so nothing
   * is reported beyond a trace. */
  if (opeu_load_scenario(game, (uint8_t)scenario, scedata, scedata_size, mapinf, mapinf_size) != 0)
    port_trace("link",
               "scensel: scenario %u: SCENARIO/MAP files shorter than the "
               "records read",
               scenario);
  std::free(scedata);
  std::free(mapinf);
}

/* ---- boundary pointers ------------------------------------------------------- *
 * aiops1..aiops5 call six routines through the global opeu_ai_unrecovered
 * (core/aiops1); svc_ai_unrecovered maps its ids to those routines.
 * Installed by main() before the entry. */
void port_install_hooks(void) {
  opeu_ai_unrecovered = svc_ai_unrecovered;
}
