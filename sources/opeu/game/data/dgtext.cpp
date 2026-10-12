// Data segment DS:3eba..3eeb (50 bytes): the initialised variables of the text output (core/textout) and the
// keyboard's idle hook (io/keyboard), after the data of the file and VGA libraries; DGFONT continues at DS:3eec.
#include "data/dgtext.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::dgtext {

auto InitialTextState(DataSegment& data) -> void {
  data.text_right  = 80;
  data.text_bottom = 200;
  data.text_height = 16;
  data.text_ink    = 0x07;
  data.saved_ink   = 0x07;
}
} // namespace opeu::game::detail::dgtext
