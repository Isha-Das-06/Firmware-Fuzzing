/* Minimal STM32F407 register definitions (only what this demo touches). */
#ifndef STM32F4_H
#define STM32F4_H

#include <stdint.h>

#define __IO volatile
#define REG32(addr) (*(volatile uint32_t *)(addr))

/* --- RCC (Reset & Clock Control) --- */
#define RCC_BASE        0x40023800u
#define RCC_AHB1ENR     REG32(RCC_BASE + 0x30)
#define RCC_APB1ENR     REG32(RCC_BASE + 0x40)
#define RCC_AHB1ENR_GPIOAEN (1u << 0)
#define RCC_APB1ENR_USART2EN (1u << 17)

/* --- GPIOA (USART2 is on PA2/PA3) --- */
#define GPIOA_BASE      0x40020000u
#define GPIOA_MODER     REG32(GPIOA_BASE + 0x00)
#define GPIOA_AFRL      REG32(GPIOA_BASE + 0x20)

/* --- USART2 --- */
#define USART2_BASE     0x40004400u
#define USART2_SR       REG32(USART2_BASE + 0x00)
#define USART2_DR       REG32(USART2_BASE + 0x04)
#define USART2_BRR      REG32(USART2_BASE + 0x08)
#define USART2_CR1      REG32(USART2_BASE + 0x0C)

#define USART_SR_RXNE   (1u << 5)
#define USART_SR_TXE    (1u << 7)
#define USART_CR1_RE    (1u << 2)
#define USART_CR1_TE    (1u << 3)
#define USART_CR1_UE    (1u << 13)

#endif /* STM32F4_H */
