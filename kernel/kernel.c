// kernel/kernel.c
// TouchOS Kernel - Main kernel code
// Where all the magic (and bugs) happen
//
// Created by: floof<3
// Special thanks to: XansiVA (github.com/XansiVA) for helping get this project started
// You da real MVP

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../drivers/serial.h"  // For debug output (so we can actually see what's going on)
#include "pmm.h"   // Physical memory manager
#include "heap.h"  // Heap allocator (kmalloc/kfree)

// Forward declarations for stub functions that aren't implemented yet
// (We removed pmm_init, heap_init since those are now real)
void gdt_init(void);
void idt_init(void);
void pic_init(void);
void apic_init(void);
void scheduler_init(void);
void vmm_init(void);
void vmm_map_page(uint64_t* pml, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void pci_scan(void);
void usb_init(void);
void graphics_init(void* gop_ptr);
void touch_init(void);
void wm_init(void);
void vfs_init(void);
void pkg_init(void);

// Boot parameters structure (what the bootloader gives us)
typedef struct {
    uint64_t memory_size;
    void* gop;  // Graphics Output Protocol pointer (UEFI GOP) - basically the framebuffer
    uint8_t gap[256];  // Reserved space (idk what this is for but it's here)
} BootParams;

// Memory management constants
#define PAGE_SIZE 4096  // 4KB pages because that's what x86_64 uses
#define PAGE_PRESENT (1 << 0)  // page is in memory
#define PAGE_WRITE (1 << 1)  // page is writable

// Global variables (yes I know globals are bad but this is a kernel so shut up)
static uint64_t* kernel_pml4 = NULL;
static uint64_t total_memory = 0;

// Kernel panic handler (oh shit moment)
void kernel_panic(const char* message, uint32_t error_code) {
    // TODO: Display error message on screen
    // For now, just halt the CPU and cry
    (void)message;  // Suppress unused parameter warning (compiler is so annoying)
    (void)error_code;
    
    __asm__ volatile("cli; hlt");  // disable interrupts and halt
    while(1) {
        __asm__ volatile("hlt");  // stay halted forever lmao
    }
}

// GDT (Global Descriptor Table) initialization
void gdt_init(void) {
    // TODO: Set up 64-bit GDT with code and data segments
    // Already done in boot64.asm, but can be reloaded here if needed
    // honestly GDT is kinda pointless in 64-bit mode but we need it anyway
}

// IDT (Interrupt Descriptor Table) initialization  
void idt_init(void) {
    // TODO: Set up IDT with 256 entries
    // Install exception handlers (divide by zero, page fault, etc.)
    // Install IRQ handlers
    // this is where we tell the CPU what to do when shit hits the fan
}

// PIC (Programmable Interrupt Controller) initialization
void pic_init(void) {
    // TODO: Remap PIC to avoid conflicts with CPU exceptions
    // Master PIC: IRQ 0-7 -> INT 32-39
    // Slave PIC: IRQ 8-15 -> INT 40-47
    // the PIC is old as fuck but we still need to configure it
}

// APIC (Advanced Programmable Interrupt Controller) initialization
void apic_init(void) {
    // TODO: Detect and initialize Local APIC and IO APIC
    // Disable legacy PIC (bye bye old hardware)
    // Configure APIC timer for preemptive multitasking
    // APIC is way better than PIC but also way more complicated
}

// Scheduler initialization (multitasking go brrr)
void scheduler_init(void) {
    // TODO: Initialize task list
    // Set up timer interrupt for context switching
    // Create idle task (the task that does nothing when there's nothing to do)
}

// VMM (Virtual Memory Manager) initialization
void vmm_init(void) {
    // Allocate PML4 (Page Map Level 4) table
    kernel_pml4 = (uint64_t*)0x1000;  // Temporary physical address (this is kinda sketchy but whatever)
    
    // Clear PML4 (zero it out so we start fresh)
    for (int i = 0; i < 512; i++) {
        kernel_pml4[i] = 0;
    }
    
    // Identity map first 4GB for kernel and devices
    // (virtual addr = physical addr, makes life easier)
    for (uint64_t addr = 0; addr < 0x100000000; addr += 0x200000) {
        // Using 2MB pages for simplicity (requires PAGE_SIZE flag)
        vmm_map_page(kernel_pml4, addr, addr, PAGE_PRESENT | PAGE_WRITE | PAGE_SIZE); //was missing an underscore.
    }
    
    // Load CR3 with new page table (tell the CPU about our fancy new page tables)
    __asm__ volatile("mov %0, %%cr3" :: "r"(kernel_pml4));
}

// Map a virtual page to physical page
void vmm_map_page(uint64_t* pml, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) { //Flags?? more like.. FAGS LMAOOO :3
    (void)pml;
    (void)virtual_addr;
    (void)physical_addr;
    (void)flags;
    
    // TODO: Implement 4-level page table mapping
    // Extract indices from virtual address (bit twiddling time)
    // Allocate intermediate tables if needed (PDPT, PD, PT)
    // Set leaf entry to physical address with flags
    // x86_64 paging is a pain in the ass btw
}

// PCI bus scanning (find all the hardware)
void pci_scan(void) {
    // TODO: Scan PCI configuration space
    // Detect USB controllers (XHCI, EHCI)
    // Detect graphics cards
    // Detect network cards
    // basically just poke at memory-mapped registers until we find stuff
}

// USB stack initialization
void usb_init(void) {
    // TODO: Initialize USB host controllers
    // Enumerate USB devices (what's plugged in?)
    // Load USB HID driver for touchscreen
    // USB is a fucking nightmare protocol but we need it
}

// Graphics initialization
void graphics_init(void* gop_ptr) {
    (void)gop_ptr;
    // TODO: Set up framebuffer from GOP
    // Initialize double buffering (so we don't get screen tearing)
    // Set up hardware acceleration if available (make it go zoom)
}

// Touchscreen driver initialization
void touch_init(void) {
    // TODO: Initialize Acer T230H touchscreen
    // Set up USB HID protocol (Human Interface Device)
    // Calibrate touch coordinates (make sure touches are accurate)
    // Enable multi-touch support (pinch to zoom baby)
}

// Window Manager initialization
void wm_init(void) {
    // TODO: Create root window
    // Initialize window list
    // Set up touch gesture recognition (swipes, pinches, etc.)
    // Create on-screen keyboard (pops up when you need it)
}

// VFS (Virtual File System) initialization
void vfs_init(void) {
    // TODO: Initialize VFS layer
    // Mount root filesystem
    // Set up device nodes (/dev)
    // this is the abstraction layer so we can support multiple filesystems
}

// Package manager initialization
void pkg_init(void) {
    // TODO: Initialize package database
    // Set up HTTP client for package downloads
    // Configure package repository URL
    // our own package manager instead of .deb because we're cool like that
}

// Main kernel entry point (called from boot64.asm in 64-bit mode)
// RDI contains pointer to multiboot info struct
void kernel_main(void* multiboot_info) {
    (void)multiboot_info;

    // Just halt - no serial, no memory init, nothing
    while(1) {
        __asm__ volatile("hlt");
    }
}
