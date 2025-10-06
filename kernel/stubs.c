#include "memory.h"
#include "interrupts.h"
#include "scheduler.h"
#include "vfs.h"

// Memory stubs
void spin_lock(spinlock_t* lock) { (void)lock; }
void spin_unlock(spinlock_t* lock) { (void)lock; }

void* memset(void* dest, int val, size_t count) {
    unsigned char* d = dest;
    while (count--) *d++ = val;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (count--) *d++ = *s++;
    return dest;
}

void* kmalloc(size_t size) {
    (void)size;
    return (void*)0x200000; // Temporary - return fixed address
}

void kfree(void* ptr) { (void)ptr; }

// Interrupt stubs
void gdt_init(void) {}
void idt_init(void) {}
void pic_init(void) {}
void apic_init(void) {}
void sti(void) { __asm__ volatile("sti"); }

// Scheduler stubs
void scheduler_init(void) {}
void scheduler_start(void) { while(1) __asm__ volatile("hlt"); }

// VFS stubs  
void vfs_init(void) {}
void initrd_load(void) {}
void process_create(const char* path) { (void)path; }

// Other missing functions
void pci_scan(void) {}
void usb_init(void) {}
void graphics_init(void* gop) { (void)gop; }
