/* Link: core/requests.c (1270:08e0 timed and pending side requests).
 * Services (host branch comment): 0 request_valid(side,request) 1270:036e,
 * 1/2/3 request_action0/1/2(side,request) 1270:05ac/06bc/0876,
 * 4 request_complete(side) 1270:0242, 5 request_start(side,request)
 * 1270:047c, 6 request_fallback(side,request) 1270:0570. */
#include "c2_common.h"

uint16_t link_requests(uint8_t *ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    uint16_t r = 0;
    (void)c; (void)d;
    switch (id) {
    case 0: r = opeu_request_valid(ds, link_reqvalid, (uint8_t)a, b); break;     /* 1270:036e */
    case 1: opeu_request_action0(ds, link_reqres, (uint8_t)a, b); break;         /* 1270:05ac */
    case 2: opeu_request_action1(ds, link_reqequip, (uint8_t)a, b); break;       /* 1270:06bc */
    case 3: opeu_request_action2(ds, link_reqspawn, (uint8_t)a, b); break;       /* 1270:0876 */
    case 4: opeu_request_complete(ds, (uint8_t)a); break;                        /* 1270:0242 */
    case 5: opeu_request_start(ds, link_reqpend, (uint8_t)a, b); break;          /* 1270:047c */
    case 6: opeu_request_fallback(ds, link_reqpend, (uint8_t)a, b); break;       /* 1270:0570 */
    default: port_unhandled("requests", id, 4);
    }
    return r;
}
