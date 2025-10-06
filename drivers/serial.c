// drivers/serial.c
// Serial port driver for debug output
// Lets us scream into the void and actually get a response back
// Without this, debugging a kernel is like trying to fix a car with your eyes closed

#include "serial.h"

// Initialize serial port (configure it so we can actually use it)
void serial_init(void) {
    // Disable all interrupts on COM1 (we're polling like it's 1985)
    // Interrupts are cool but we don't have an IDT set up yet so fuck that
    outb(COM1 + 1, 0x00);
    
    // Enable DLAB (Divisor Latch Access Bit) so we can set baud rate
    // Basically tells the serial controller "hey I wanna change your speed"
    outb(COM1 + 3, 0x80);
    
    // Set divisor to 3 (38400 baud) - lower divisor = faster speed
    // 38400 is plenty fast for debug messages, no need to go crazy
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    
    // Disable DLAB and configure: 8 bits, no parity, one stop bit
    // This is the standard serial configuration everyone uses (8N1)
    outb(COM1 + 3, 0x03);
    
    // Enable FIFO with 14-byte threshold
    // FIFO = First In First Out buffer (helps prevent data loss when we spam messages)
    outb(COM1 + 2, 0xC7);
    
    // Mark data terminal ready, signal request to send and enable aux output #2
    // Basically saying "I'm ready to send data, please don't ignore me"
    // These flags are ancient but still required for compatibility
    outb(COM1 + 4, 0x0B);
    
    // Enable interrupts (even though we disabled them earlier... serial ports are weird)
    // We'll use this later when we have proper interrupt handling
    outb(COM1 + 1, 0x01);
}

// Check if the transmit buffer is empty (can we send data yet?)
static int serial_transmit_empty(void) {
    // Bit 5 of the line status register = transmitter holding register empty
    return inb(COM1 + 5) & 0x20;
}

// Write a single character to serial port
void serial_putchar(char c) {
    // Wait for transmit buffer to be empty (busy waiting like it's the 90s)
    // This is inefficient but simple and works fine for debug output
    while (!serial_transmit_empty())
        ;  // Spin spin spin (wastes CPU but we don't care for debug messages)
    
    // Actually send the character
    outb(COM1, c);
}

// Write a null-terminated string to serial port
void serial_write(const char* str) {
    // Just loop through and send each character
    // Simple but effective (like a hammer - sometimes the simple tool is the best tool)
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putchar(str[i]);
    }
}
