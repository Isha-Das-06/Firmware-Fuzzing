/*
 * startup.c - Vector table, reset handler, and the crash-marker fault handler.
 *
 * The HardFault handler is the on-target crash detector: when the planted
 * parser bug corrupts the stack and the function returns into garbage, the
 * Cortex-M core faults and lands here. It prints a unique marker over USART2
 * that the Renode test greps for -- proving the crash reproduces on the
 * actual firmware, not just in the native fuzzer.
 */
#include <stdint.h>
#include "stm32f4.h"

extern int  main(void);
extern uint32_t _sdata, _edata, _sbss, _ebss, _etext, _estack;

void Reset_Handler(void);
void Default_Handler(void);
void HardFault_Handler(void);

/* Freestanding libc primitives: GCC emits implicit calls to these, but we
 * build -nostdlib so we supply our own. Kept simple and Thumb-native. */
void *memcpy(void *dst, const void *src, unsigned long n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *memset(void *dst, int c, unsigned long n)
{
    unsigned char *d = dst;
    while (n--) {
        *d++ = (unsigned char)c;
    }
    return dst;
}

/* Self-contained UART putc: usable even after the stack is corrupted, because
 * it touches only registers and no C stack state. */
static void fault_putc(char c)
{
    while (!(USART2_SR & USART_SR_TXE)) { }
    USART2_DR = (uint32_t)(uint8_t)c;
}

static void fault_puts(const char *s)
{
    while (*s) {
        fault_putc(*s++);
    }
}

void HardFault_Handler(void)
{
    /* Unique, greppable crash marker. */
    fault_puts("\r\n!!FAULT:HARDFAULT!!\r\n");
    for (;;) { }
}

/*
 * Stack-smashing detector.
 *
 * The firmware is built with -fstack-protector-all, so every function gets a
 * canary between its locals and its saved return address. When the planted
 * overflow in decode_packet() runs past its 16-byte buffer it clobbers the
 * canary; the epilogue check catches this and calls __stack_chk_fail() BEFORE
 * the corrupted return address is used -- a deterministic on-target trap for
 * exactly the bug the native fuzzer reports. We supply both symbols ourselves
 * because the firmware links -nostdlib.
 */
uintptr_t __stack_chk_guard = 0xDEADBEEFu;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
    fault_puts("\r\n!!FAULT:STACK-SMASH!!\r\n");
    for (;;) { }
}

void Reset_Handler(void)
{
    /* Copy .data from FLASH to RAM. */
    uint32_t *src = &_etext;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    /* Zero .bss. */
    for (dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0;
    }
    main();
    for (;;) { }
}

void Default_Handler(void)
{
    for (;;) { }
}

/* Cortex-M vector table. Renode loads the initial SP and PC from here. */
__attribute__((section(".isr_vector"), used))
void (*const g_vectors[])(void) = {
    (void (*)(void))(&_estack), /* 0x00: initial stack pointer */
    Reset_Handler,              /* 0x04: reset                 */
    Default_Handler,            /* 0x08: NMI                   */
    HardFault_Handler,          /* 0x0C: HardFault             */
    Default_Handler,            /* 0x10: MemManage             */
    Default_Handler,            /* 0x14: BusFault              */
    Default_Handler,            /* 0x18: UsageFault            */
    0, 0, 0, 0,                 /* reserved                    */
    Default_Handler,            /* 0x2C: SVCall                */
    Default_Handler,            /* 0x30: Debug Monitor         */
    0,                          /* reserved                    */
    Default_Handler,            /* 0x38: PendSV                */
    Default_Handler,            /* 0x3C: SysTick               */
};
