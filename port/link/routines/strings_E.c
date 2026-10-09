/* core/strings.c 09cc:01d0 string_copy (AX source, BX destination) ->
 * destination, reached by address from btlcmdr (whose source-order words
 * link_btlcmdr converts). */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_string_copy)
{
    return opeu_string_copy(ds, ARG(1), ARG(0));
}
