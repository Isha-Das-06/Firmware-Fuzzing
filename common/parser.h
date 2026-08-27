/*
 * parser.h - Shared serial packet decoder.
 *
 * This exact code is compiled into TWO targets:
 *   1. the STM32F4 firmware (runs on the emulated board in Renode)
 *   2. the libFuzzer harness (runs natively under ASan in CI)
 *
 * Because both use the same translation unit, a crash the fuzzer finds is a
 * crash that exists in the shipping firmware -- not an approximation of it.
 *
 * Wire framing (a byte stream, as it would arrive over UART):
 *
 *     +------+------+---------------------+
 *     | SYNC | LEN  | PAYLOAD (LEN bytes) |
 *     +------+------+---------------------+
 *       0xAA  0..255
 *
 * The decoder is a byte-at-a-time state machine so it behaves identically
 * whether bytes trickle in over a real UART or arrive as one fuzz buffer.
 */
#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdint.h>

#define PKT_SYNC 0xAAu

/* Callback invoked once a full, framed packet has been received. */
typedef void (*packet_cb_t)(const uint8_t *payload, uint8_t len, void *user);

typedef enum {
    ST_SYNC = 0, /* waiting for the 0xAA start byte      */
    ST_LEN,      /* next byte is the payload length      */
    ST_PAYLOAD   /* collecting `len` payload bytes        */
} parser_state_t;

typedef struct {
    parser_state_t state;
    uint8_t        len;   /* declared payload length            */
    uint8_t        idx;   /* payload bytes collected so far      */
    uint8_t        payload[256]; /* assembly buffer (max frame)  */
    packet_cb_t    cb;
    void          *user;
} parser_t;

/* Reset the state machine and register the completed-packet callback. */
void parser_init(parser_t *p, packet_cb_t cb, void *user);

/* Feed one received byte. Drives framing; fires the callback on a full frame. */
void parser_feed(parser_t *p, uint8_t byte);

/*
 * Decode one fully-framed packet's payload.
 *
 * This is the routine under test. It performs the classic mistake of
 * trusting an attacker-controlled length field when copying into a
 * fixed-size stack buffer. Kept in its own function so the bug has one
 * obvious home and one obvious fix.
 *
 * Returns an XOR checksum of the payload (so the copy is actually used and
 * not optimized away).
 */
uint8_t decode_packet(const uint8_t *payload, uint8_t len);

#endif /* PARSER_H */
