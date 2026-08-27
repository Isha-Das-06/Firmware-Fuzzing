/*
 * main.c - USART2 serial command loop for the STM32F4 (emulated in Renode).
 *
 * Reads bytes off USART2, feeds them into the SHARED parser state machine
 * (../common/parser.c), and prints a short reply for each completed packet.
 * A malformed packet whose length field exceeds the decoder's 16-byte scratch
 * buffer overflows the stack and faults the core -> HardFault_Handler prints
 * the crash marker.
 */
#include <stdint.h>
#include "stm32f4.h"
#include "parser.h"

static void uart_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2/PA3 to alternate-function mode (AF7 = USART2). Harmless in Renode,
     * correct on real silicon. */
    GPIOA_MODER &= ~((3u << (2 * 2)) | (3u << (3 * 2)));
    GPIOA_MODER |=  ((2u << (2 * 2)) | (2u << (3 * 2)));
    GPIOA_AFRL  &= ~((0xFu << (4 * 2)) | (0xFu << (4 * 3)));
    GPIOA_AFRL  |=  ((7u << (4 * 2)) | (7u << (4 * 3)));

    USART2_BRR = 0x0683;                 /* ~9600 @ 16MHz (Renode ignores) */
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart_putc(char c)
{
    while (!(USART2_SR & USART_SR_TXE)) { }
    USART2_DR = (uint32_t)(uint8_t)c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

static uint8_t uart_getc(void)
{
    while (!(USART2_SR & USART_SR_RXNE)) { }
    return (uint8_t)(USART2_DR & 0xFF);
}

static void put_hex2(uint8_t b)
{
    static const char h[] = "0123456789ABCDEF";
    uart_putc(h[(b >> 4) & 0xF]);
    uart_putc(h[b & 0xF]);
}

/* Fires once per fully-framed packet. */
static void on_packet(const uint8_t *payload, uint8_t len, void *user)
{
    (void)user;
    uint8_t sum = decode_packet(payload, len); /* <-- vulnerable decode */
    uart_puts("OK:");
    put_hex2(sum);
    uart_puts("\r\n");
}

int main(void)
{
    parser_t parser;

    uart_init();
    parser_init(&parser, on_packet, NULL);

    uart_puts("READY\r\n");

    for (;;) {
        parser_feed(&parser, uart_getc());
    }
}
