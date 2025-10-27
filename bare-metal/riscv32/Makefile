# Makefile for V4 VM RISC-V 32-bit bare-metal test

# Cross-compiler toolchain
# Note: Using riscv64 toolchain with -march=rv32 for 32-bit target
CROSS_COMPILE ?= riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# Compiler flags
CFLAGS = -march=rv32ima -mabi=ilp32 -mcmodel=medany \
         -nostdlib -nostartfiles -ffreestanding \
         -O2 -Wall -Wextra \
         -I../../include

ASFLAGS = -march=rv32ima -mabi=ilp32

LDFLAGS = -m elf32lriscv -T link.ld -nostdlib

# Source files
SRCS = main_simple.c
OBJS = start.o $(SRCS:.c=.o)

# Output
TARGET = bare-metal-test
ELF = $(TARGET).elf
BIN = $(TARGET).bin

# QEMU command
QEMU = qemu-system-riscv32
QEMU_FLAGS = -M virt -nographic -bios none -kernel $(ELF)

.PHONY: all clean run dump

all: $(ELF) $(BIN)

$(ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $<

dump: $(ELF)
	$(OBJDUMP) -d $<

run: $(ELF)
	$(QEMU) $(QEMU_FLAGS)

clean:
	rm -f start.o main.o main_simple.o core.o memory.o $(ELF) $(BIN)
