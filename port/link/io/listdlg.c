/* Link: io/listdlg.c (0a8e list dialogs). LISTDLG raises the same list
 * boundary as LISTUI ("Host model and event numbering: see LISTUI"), and
 * passes its callback on to the LISTUI routines it calls; every event is
 * served by link_listui (port/link/io/listui.c). */
#include "link.h"

uint16_t link_listdlg(uint8_t *ds, uint16_t kind, uint16_t *args, uint16_t count)
{
    return link_listui(ds, kind, args, count);
}
