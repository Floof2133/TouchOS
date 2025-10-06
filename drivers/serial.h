// drivers/serial.h
// Serial port driver for debug output
// This lets us yell at the void (or more specifically, at the terminal)
// Basically the only way to debug shit when you don't have graphics yet

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

// COM1 port address (the classic serial port every x86 machine has)
// Even modern systems emulate this in QEMU/real hardware for backwards compatibility
#define COM1 0x3F8

// Initialize serial port (call this once at boot before using serial_write)
void serial_init(void);

// Write a single character to serial port
void serial_putchar(char c);

// Write a null-terminated string to serial port
// This is your printf for now (until we get proper console output working)
void serial_write(const char* str);

// Port I/O helpers (inline assembly because we're talking directly to hardware)
// outb = output byte, inb = input byte (classic x86 I/O instructions)
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif // SERIAL_H
