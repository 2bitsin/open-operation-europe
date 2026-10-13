// The initial values of the music state's DGROUP fields: no track and no effect loaded (FFh), the bank name and path
// buffers empty.
#include "recovered.hpp"

namespace opeu::intro::detail::odsound {

uint8_t const MUSIC_CURRENT  = 0xff;
uint8_t const EFFECT_CURRENT = 0xff;
char const    MUSIC_BANK[26] = { 0 };
char const    MUSIC_PATH[14] = { 0 };

}
