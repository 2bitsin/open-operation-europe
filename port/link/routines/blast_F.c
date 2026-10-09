/*
 * missing handler): 17c4:0866 opeu_blast_pause, the presentation callback
 * that opeu_general_casualty (units/genwound.c SHOW) passes as a far code
 * pointer to opeu_daily_show; the dialogs link reaches it through
 * port_call (dialogs event 47 present(offset,segment)). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_blast_pause)
{
    (void)args; (void)count;
    opeu_blast_pause(ds, link_blast);
    return 0;
}
