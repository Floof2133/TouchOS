# Makefile for TouchOS
# Compiles the kernel and links everything together

# Compiler and linker settings (the flags that make it actually work)
CC = gcc
LD = ld
ASM = nasm

# Compiler flags (tell GCC how to compile for bare metal)
CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -O2 -Wall -Wextra

# Linker flags (tell LD how to link the kernel)
LDFLAGS = -n -T kernel/linker.ld

# Object files (all the .o files we need to link together)
OBJS = kernel/kernel.o kernel/pmm.o kernel/heap.o drivers/serial.o kernel/boot/boot64.o

# Default target (what happens when you just type 'make')
all: kernel.elf

# Compile kernel.c to kernel.o
kernel/kernel.o: kernel/kernel.c drivers/serial.h
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel/kernel.o

# Compile serial.c to serial.o  
drivers/serial.o: drivers/serial.c drivers/serial.h
	$(CC) $(CFLAGS) -c drivers/serial.c -o drivers/serial.o

# Assemble boot64.asm to boot64.o
kernel/boot/boot64.o: kernel/boot/boot64.asm
	$(ASM) -f elf64 kernel/boot/boot64.asm -o kernel/boot/boot64.o

# Link everything into kernel.elf (the final kernel binary)
kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

# Clean up (delete all compiled files so we can rebuild from scratch)
clean:
	rm -f kernel/kernel.o drivers/serial.o kernel/boot/boot64.o kernel.elf

# Phony targets (these aren't actual files, just commands)
.PHONY: all clean
