# Bare-metal STM32F407 firmware build (Cortex-M4).
#
# Shares ../common/parser.c with the fuzzer, so the bug the fuzzer finds is
# the bug that runs here.

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

BUILD   := build
TARGET  := $(BUILD)/firmware

ARCH    := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS  := $(ARCH) -O0 -g3 -ffreestanding -fstack-protector-all \
           -ffunction-sections -fdata-sections \
           -Wall -Wextra -I. -I../common
LDFLAGS := $(ARCH) -T stm32f407.ld -nostdlib -Wl,--gc-sections \
           -Wl,-Map=$(TARGET).map

SRCS    := startup.c main.c ../common/parser.c

.PHONY: all clean
all: $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(SRCS) stm32f407.ld ../common/parser.h | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
