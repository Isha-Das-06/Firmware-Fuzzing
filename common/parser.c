/*
 * parser.c - Shared serial packet decoder (see parser.h).
 *
 * Contains one deliberately planted, realistic vulnerability so the whole
 * pipeline has something to find. See decode_packet().
 */
#include "parser.h"
#include <string.h>

void parser_init(parser_t *p, packet_cb_t cb, void *user)
{
    memset(p, 0, sizeof(*p));
    p->state = ST_SYNC;
    p->cb    = cb;
    p->user  = user;
}

void parser_feed(parser_t *p, uint8_t byte)
{
    switch (p->state) {
    case ST_SYNC:
        if (byte == PKT_SYNC) {
            p->state = ST_LEN;
        }
        break;

    case ST_LEN:
        p->len   = byte;
        p->idx   = 0;
        p->state = (byte == 0) ? ST_SYNC : ST_PAYLOAD;
        if (byte == 0 && p->cb) {
            p->cb(p->payload, 0, p->user); /* empty packet is valid */
        }
        break;

    case ST_PAYLOAD:
        p->payload[p->idx++] = byte; /* idx < len <= 255 < sizeof(payload) */
        if (p->idx >= p->len) {
            if (p->cb) {
                p->cb(p->payload, p->len, p->user);
            }
            p->state = ST_SYNC;
        }
        break;
    }
}

uint8_t decode_packet(const uint8_t *payload, uint8_t len)
{
    uint8_t scratch[16];
    uint8_t sum = 0;

    /*
     * BUG (CWE-121: stack-based buffer overflow):
     * `len` comes straight off the wire and can be up to 255, but `scratch`
     * holds only 16 bytes. There is no bounds check before the copy.
     *
     * The fix is a single line:
     *     if (len > sizeof(scratch)) len = sizeof(scratch);
     * (or reject the frame outright). It is commented out so the fuzzer has a
     * crash to discover; flip it on to watch CI go green.
     */
    /* if (len > sizeof(scratch)) len = sizeof(scratch); */
    memcpy(scratch, payload, len);

    for (uint8_t i = 0; i < len && i < sizeof(scratch); i++) {
        sum ^= scratch[i];
    }
    return sum;
}
