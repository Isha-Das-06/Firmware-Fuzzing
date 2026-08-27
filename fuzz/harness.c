/*
 * harness.c - libFuzzer entry point.
 *
 * Drives the SAME parser state machine the firmware uses, one fuzzer-supplied
 * buffer at a time. Built natively with clang's -fsanitize=fuzzer,address so
 * the stack overflow in decode_packet() is caught deterministically with a
 * clean AddressSanitizer report -- the fast gate that runs on every push.
 *
 * Build:  see fuzz/Makefile  (or the top-level Makefile `fuzz` target)
 * Run:    ./fuzz/build/fuzz_parser -runs=200000 corpus/
 */
#include <stddef.h>
#include <stdint.h>

#include "parser.h"

/* Consume completed packets so the callback path is exercised too. */
static void on_packet(const uint8_t *payload, uint8_t len, void *user)
{
    volatile uint8_t sink = decode_packet(payload, len);
    (void)sink;
    (void)user;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    parser_t p;
    parser_init(&p, on_packet, NULL);
    for (size_t i = 0; i < size; i++) {
        parser_feed(&p, data[i]);
    }
    return 0;
}
